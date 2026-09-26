"""Exercise command-output draining with real bounded subprocesses."""

import logging
import subprocess
import sys
import threading
from unittest import TestCase
from unittest.mock import Mock, patch

from harmony_service.exceptions import (
    InternalError,
    InvalidParameter,
    MissingParameter,
    NoMatchingData,
    NoPolygonFound,
)
from harmony_service.utilities import execute_command


class TestExecuteCommandPipes(TestCase):
    """Keep both pipes drained without relying on installed subsetter binaries."""

    def setUp(self):
        self.processes = []
        self.timers = []
        self.watchdog_fired = threading.Event()
        self.logger = Mock(spec=logging.Logger)

    def tearDown(self):
        for timer in self.timers:
            timer.cancel()
        for process in self.processes:
            if process.poll() is None:
                process.kill()
            process.wait(timeout=5)
            for stream in (process.stdout, process.stderr):
                if stream is not None:
                    stream.close()

    def launch(self, code, *, already_exited=False, args=()):
        def popen(*positional, **kwargs):
            process = subprocess.Popen(*positional, **kwargs)
            self.processes.append(process)
            if already_exited:
                process.wait(timeout=5)
            else:

                def stop_stalled_child():
                    if process.poll() is None:
                        self.watchdog_fired.set()
                        process.kill()

                # Bound the regression on the old implementation rather than
                # leaving a blocked process behind when stderr fills its pipe.
                timer = threading.Timer(5, stop_stalled_child)
                timer.daemon = True
                self.timers.append(timer)
                timer.start()
            return process

        command = [sys.executable, "-c", code, *args]
        with patch("harmony_service.utilities.Popen", side_effect=popen):
            try:
                result = execute_command(command, self.logger)
            finally:
                self.assertFalse(
                    self.watchdog_fired.is_set(),
                    "command blocked while draining output",
                )
        for process in self.processes:
            self.assertIsNotNone(process.returncode)
            self.assertTrue(process.stdout.closed)
            self.assertTrue(process.stderr.closed)
        return result

    def test_stderr_larger_than_pipe_buffer_does_not_block_stdout(self):
        for order in ("stderr-first", "alternating"):
            with self.subTest(order=order):
                self.logger.reset_mock()
                if order == "stderr-first":
                    code = "import sys; sys.stderr.write('E'*1048576+'\\n'); sys.stderr.flush(); print('finished')"
                    expected_errors = ["E" * 1048576 + "\n"]
                    expected_info = ["finished\n"]
                else:
                    code = (
                        "import sys\n"
                        "for i in range(4):\n"
                        " sys.stderr.write('E'*262144+'\\n'); sys.stderr.flush()\n"
                        " print(i, flush=True)\n"
                    )
                    expected_errors = ["E" * 262144 + "\n"] * 4
                    expected_info = [f"{i}\n" for i in range(4)]
                self.assertIsNone(self.launch(code))
                self.assertEqual(
                    [c.args[0] for c in self.logger.error.call_args_list],
                    expected_errors,
                )
                self.assertEqual(
                    [c.args[0] for c in self.logger.info.call_args_list[1:]],
                    expected_info,
                )

    def test_already_exited_commands_still_log_both_pipes(self):
        for status in (0, 3):
            with self.subTest(status=status):
                self.logger.reset_mock()
                code = f"import sys; print('normal'); print('diagnostic', file=sys.stderr); sys.exit({status})"
                if status:
                    with self.assertRaises(NoMatchingData):
                        self.launch(code, already_exited=True)
                else:
                    self.assertIsNone(self.launch(code, already_exited=True))
                self.logger.info.assert_any_call("normal\n")
                self.logger.error.assert_called_once_with("diagnostic\n")

    def test_error_codes_keep_their_exceptions_after_logging(self):
        cases = (
            (1, InvalidParameter),
            (2, MissingParameter),
            (3, NoMatchingData),
            (6, NoPolygonFound),
            (17, InternalError),
        )
        for status, error in cases:
            with self.subTest(status=status):
                self.logger.reset_mock()
                code = f"import sys; print('context'); print('error detail', file=sys.stderr); sys.exit({status})"
                with self.assertRaises(error) as caught:
                    self.launch(code)
                self.assertEqual(caught.exception.exit_status, status)
                self.logger.info.assert_any_call("context\n")
                self.logger.error.assert_called_once_with("error detail\n")

    def test_multiline_unicode_and_unterminated_output_are_preserved(self):
        self.launch(
            "import sys; sys.stdout.write('first\\n\\nlast café'); sys.stderr.write('warning\\nlast warning')"
        )
        self.assertEqual(
            [c.args[0] for c in self.logger.info.call_args_list[1:]],
            ["first\n", "\n", "last café"],
        )
        self.assertEqual(
            [c.args[0] for c in self.logger.error.call_args_list],
            ["warning\n", "last warning"],
        )

    def test_empty_output_does_not_add_empty_log_messages(self):
        self.assertIsNone(self.launch("pass"))
        self.assertEqual(self.logger.info.call_count, 1)
        self.logger.error.assert_not_called()

    def test_arguments_are_still_passed_without_a_shell(self):
        argument = "folder with spaces; echo not-a-command"
        self.launch("import sys; print(sys.argv[1])", args=(argument,))
        self.logger.info.assert_any_call(argument + "\n")

    def test_missing_executable_still_raises(self):
        with self.assertRaises(FileNotFoundError):
            execute_command(
                ["/nonexistent-trajectory-subsetter-test-binary"], self.logger
            )


if __name__ == "__main__":
    import unittest

    unittest.main()
