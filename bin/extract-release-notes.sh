#!/bin/bash
##################################################################################
#
# Extract release notes for the latest version of the Trajectory Subsetter  service
#
# 2025-09-15: Updated from the Harmony Metadata Annotator
# 2025-09-23: Updated to include commit history in release notes.
#
##################################################################################

CHANGELOG_FILE="CHANGELOG.md"

## captures versions
## >## v1.0.0
## >## [v1.0.0]
VERSION_PATTERN="^## [\[]v"

## captures url links
## [v1.0.0]:https://github.com/nasa/harmony-trajectory-subsetter/releases/tag/1.0.0
LINK_PATTERN="^\[.*\]:.*https://github.com/nasa"

# Read the file and extract text between the first two occurrences of the
# VERSION_PATTERN
result=$(awk "/$VERSION_PATTERN/{c++; if(c==2) exit;} c==1" "$CHANGELOG_FILE")

# Get all commit messages since the last release (marked with a git tag). If
# there are no tags, get the full commit history of the repository.
if [[ $(git tag) ]]
then
	# There are git tags, so get the most recent one
	GIT_REF=$(git describe --tags --abbrev=0)
else
	# There are not git tags, so get the initial commit of the repository
	GIT_REF=$(git rev-list --max-parents=0 HEAD)
fi

# Retrieve the title line of all commit messages since $GIT_REF, filtering out
# those from the pre-commit-ci[bot] author and any containing the string
# "nasa/pre-commit-ci-update-config", which may result from merge commits.
GIT_COMMIT_MESSAGES=$(git log --oneline --format="%s" --perl-regexp --author='^(?!pre-commit-ci\[bot\]).*$' --grep="nasa\/pre-commit-ci-update-config" --invert-grep ${GIT_REF}..HEAD)

# Append git commit messages to the release notes:
if [[ ${GIT_COMMIT_MESSAGES} ]]
then
	result+="\n\n## Commits\n\n${GIT_COMMIT_MESSAGES}"
fi

# Print the result
echo -e "$result" |  grep -v "$VERSION_PATTERN" | grep -v "$LINK_PATTERN"
