#!/usr/bin/env python
#
# $File: test_pipeline.py $
# $LastChangedDate: 2011-06-16 20:10:41 -0500 (Thu, 16 Jun 2011) $
# $Rev: 4234 $
#
# This file is part of variant_tools, a software application to annotate,
# summarize, and filter variants for next-gen sequencing ananlysis.
# Please visit http://varianttools.sourceforge.net for details.
#
# Copyright (C) 2011 - 2013 Bo Peng (bpeng@mdanderson.org)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.
#

import os
import glob
import unittest
import subprocess
import shutil
from testUtils import ProcessTestCase, runCmd, numOfVariant, numOfSample, outputOfCmd

class TestPipeline(ProcessTestCase):
    def testCheckVariantToolsVersion(self):
        'Test command vtools init'
        self.assertSucc('vtools execute test_pipeline.pipeline checkvtools')
        self.assertFail('vtools execute test_pipeline.pipeline checkvtools_fail')
    

if __name__ == '__main__':
    unittest.main()
