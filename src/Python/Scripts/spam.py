###############################################################################
# Project:     Mahogany - cross platform e-mail GUI client
# File name:   src/Python/Scripts/spam.py: assorted spam tests
# Purpose:     show how spam filters can easily be written in Python
# Author:      Vadim Zeitlin
# Modified by:
# Created:     17.01.04
# CVS-ID:      $Id$
# Copyright:   (c) 2004 Vadim Zeitlin
# Licence:     M license
###############################################################################

import re
import Message
import MimePart
import MimeType
#import MDialogs -- comment this out to get some debugging output

# lately spams containing just a few lines of random words have become common,
# this test tries to recognize them
def isgibberish(msg):
    "Detect if the message is a gibberish spam meant to foil Bayesian filters"

    # apparently they also appear with other subjects but this form is the
    # most frequent one
    if not re.match("Re: [A-Z]{3,8},", msg.Subject()):
        #MDialogs.Status("ot the right subject form")
        return 0

    # they're also always multipart/alternative with text and html inside
    partTop = msg.GetTopMimePart()
    if partTop.GetType().GetFull() != "MULTIPART/ALTERNATIVE":
        #MDialogs.Status("Not MULTIPART/ALTERNATIVE")
        return 0

    # the text part comes first, as usual, but check for this
    partText = partTop.GetNested()
    if partText.GetType().GetFull() != "TEXT/PLAIN":
        #MDialogs.Status("Not TEXT/PLAIN")
        return 0

    # and they have exactly 3 lines of gibberish in the text part
    if partText.GetNumberOfLines() != 3:
        #MDialogs.Status("Not 3 lines")
        return 0

    # yes, it does look like spam
    return 1
