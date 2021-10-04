#!/tools/miniconda/bin/python

import os
import sys

# Logging and debbuging features for tool adapters

class Log:
    'Logging and Debugging'

    # constructor
    def __init__(self, debug, outputfile):
        
        self.debug = debug;
        
        # if debug mode is on, create a 'debug.out' file
        if self.debug == 'Y':
            self.outputfile = outputfile;
            if (os.path.isdir(self.outputfile)):
                self.filepath = self.outputfile;
            else:
                self.filepath = os.path.dirname(self.outputfile);
            self.filename = self.filepath + "/debug.out";
            self.debugfile = open(self.filename, 'w');

    # write the command to rqs log and 'debug.out' if debug mode is on
    def writeCommand(self, cmd):

        # write the command to standard error, which will be logged in rqs log
        sys.stderr.write("\ncommand line invocation: " + cmd + "\n");
        
        # if debug mode is on, write command to 'debug.out'
        if self.debug == 'Y' and os.path.isfile(self.filename):
            self.debugfile.write("command line invocation: " + cmd + "\n");

    # write message only to 'debug.out' if debug mode is on
    def writeMessage(self, message):

        # if debug mode is on, write command to 'debug.out'
        if self.debug == 'Y' and os.path.isfile(self.filename):
            self.debugfile.write(message);
      
    # write standard out and stand error
    def writeStdOutAndStdErr(self, out, err):

        # if debug mode on, write standard out and standard erro to 'debug.out'
        if self.debug == 'Y' and os.path.isfile(self.filename):
            self.debugfile.write("standard output from the above command:\n");
            self.debugfile.write(out + "\n");
            self.debugfile.write("standard error from the above command:\n");
            if err == "":
                self.debugfile.write("No error to report.\n");
            else:
                self.debugfile.write(err + "\n");

    # xml return for success case, simple or more complex content
    def XMLReturnSuccess(self, inputfile, outputfiles, download_urls = None, format = None, all_cmds = None):
        if (download_urls is None):
            print (esi_response_one % (outputfiles, inputfile, outputfiles));
        else:
            print (esi_response_two % (download_urls, inputfile, outputfiles, format, all_cmds));
    # xml return for failure case

    def XMLReturnFailure(self, exitstatus, errCode, errMsg):
        esi_response = """\
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<iesi:Exception
    xmlns:iesi="http://eosdis.nasa.gov/esi/rsp/i"
    xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
    xmlns:esi="http://eosdis.nasa.gov/esi/rsp"
    xmlns:ssw="http://newsroom.gsfc.nasa.gov/esi/rsp/ssw"
    xmlns:eesi="http://eosdis.nasa.gov/esi/rsp/e"
    xsi:schemaLocation="http://eosdis.nasa.gov/esi/rsp/i http://newsroom.gsfc.nasa.gov/esi/8.1/schemas/ESIAgentResponseInternal.xsd">
    <Code>%s</Code>
    <Message>%s Exit code %s</Message>
</iesi:Exception> """;
        print (esi_response % (errCode, errMsg, exitstatus));

    # xml return for success with warning info
    def XMLReturnSuccessWithInfo(self, warnings, inputfile, outputfiles, download_urls = None, format = None, all_cmds = None):
        idx1 = esi_response_one.find('<message>')
        idx2 = esi_response_two.find('<message>')
        if (download_urls is None):
            esi_response = esi_response_one[:idx1]  +  '<warning>%s</warning>\n    '  +  esi_response_one[idx1:]
            print (esi_response %(warnings, outputfiles, inputfile, outputfiles))
        else:
            esi_response = esi_response_two[:idx2]  +  '<warning>%s</warning>\n    '  +  esi_response_two[idx2:]
            print (esi_response % (download_urls, warnings, inputfile, outputfiles, format, all_cmds))

# XML strings for returns
esi_response_one = """\
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ns2:agentResponse xmlns:ns2="http://eosdis.nasa.gov/esi/rsp/i">
    <downloadUrls>
        <downloadUrl>
            %s
        </downloadUrl>
    </downloadUrls>
    <message>
        INFILE = %s
        OUTFILE = %s
        FORMAT = HDF5
    </message>
</ns2:agentResponse> """

esi_response_two = """\
<?xml version="1.0" encoding="UTF-8" standalone="yes"?>
<ns2:agentResponse xmlns:ns2="http://eosdis.nasa.gov/esi/rsp/i">
    <downloadUrls>
        %s
    </downloadUrls>
    <message>
        INFILE = %s
        OUTFILE = %s
        FORMAT = %s
    </message>
    <processInfo>
        <internalCommand>
            %s
        </internalCommand>
    </processInfo>
</ns2:agentResponse> """

