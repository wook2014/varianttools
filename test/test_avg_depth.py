#!/usr/bin/env python
#
# $File: test_import_vcf.py $
# $LastChangedDate: 2011-06-16 20:10:41 -0500 (Thu, 16 Jun 2011) $
# $Rev: 4234 $
#
# This file is part of variant_tools, a software application to annotate,
# summarize, and filter variants for next-gen sequencing ananlysis.
# Please visit http://variant_tools.sourceforge.net # for details.
#
# Copyright (C) 2004 - 2010 Bo Peng (bpeng@mdanderson.org)
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
from testUtils import ProcessTestCase, runCmd, initTest

class TestAvgDepth(ProcessTestCase):
    def setUp(self):
        'Create a project'
        initTest(5)
        runCmd('vtools sample_stat variant --num num --freq freq --depth depth')
    def removeProj(self):
        runCmd('vtools remove project')
    def testAvgDepth(self):
        'Test command vtools_report avg_depth'
        self.assertFail('vtools_report avg_depth')
        self.assertSucc('vtools_report avg_depth -h')
        # error: argument -n/--num_field is required
        self.assertFail('vtools_report avg_depth variant non_existing_column')
        # error: argument -d/--depth_field is required
        self.assertFail('vtools_report avg_depth variant -n num')
        self.assertSucc('vtools_report avg_depth variant -n num -d depth')
        self.assertSucc('vtools_report avg_depth variant -d depth -n num')
        # error: argument -d/--depth_field is required
        self.assertFail('vtools_report avg_depth variant -n num --group_by num')
        self.assertSucc('vtools_report avg_depth variant -n num -d depth --group_by num')
        self.assertSucc('vtools_report avg_depth variant -n num -d depth --group_by depth')

if __name__ == '__main__':
    unittest.main()
