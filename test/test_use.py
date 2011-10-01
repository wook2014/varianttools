#!/usr/bin/env python
#
# $File: test_use.py $
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

class TestUse(ProcessTestCase):
    def setUp(self):
        'Create a project'
        initTest(5)
    
    def removeProj(self):
        runCmd('vtools remove project')

    def testUse(self):
        'Test command vtools use'
        self.assertFail('vtools use')
        self.assertSucc('vtools use -h')
        self.assertFail('vtools use non_existing_file.ann')
        self.assertSucc('vtools use ann/testNSFP.ann')
        self.assertSucc('vtools use ann/testNSFP.ann --files ann/testNSFP.zip')
        self.assertFail('vtools use ann/testNSFP.ann --files ann/non_existing_file.zip')
        self.assertSucc('vtools use ann/testNSFP.DB.gz')

    def testThousandGenomes(self):
        'Test variants in thousand genomes'
        # no hg19
        self.assertFail('vtools use ann/testThousandGenomes.ann --files ann/testThousandGenomes.vcf.head')
        # liftover
        self.assertSucc('vtools liftover hg19')
        # ok now
        self.assertSucc('vtools use ann/testThousandGenomes.ann --files ann/testThousandGenomes.vcf.head')
        # 137 lines, 9 with two alt
        self.assertOutput('vtools execute "SELECT COUNT(1) FROM testThousandGenomes.testThousandGenomes;"', '146\n')
        # test the handling of 10327   CT   C,CA
        self.assertOutput('vtools execute "SELECT chr, pos, ref, alt FROM testThousandGenomes.testThousandGenomes WHERE pos=10328;"',
            '1\t10328\tT\t-\n1\t10328\tT\tA\n')
        # test the handling of 83934	rs59235392	AG	A,AGAAA	.
        self.assertOutput('vtools execute "SELECT chr, pos, ref, alt FROM testThousandGenomes.testThousandGenomes WHERE pos=83935;"',
            '1\t83935\tG\t-\n')
        self.assertOutput('vtools execute "SELECT chr, pos, ref, alt FROM testThousandGenomes.testThousandGenomes WHERE pos=83936;"',
            '1\t83936\t-\tAAA\n')
        #
        # Now, let us import vcf regularly.
        runCmd('vtools init test --force')
        self.assertSucc('vtools import --format vcf ann/testThousandGenomes.vcf.head --build hg19')
        self.assertSucc('vtools use ann/testThousandGenomes.ann')
        # do we have all the variants matched up?
        self.assertOutput('vtools select variant -c', '146\n')
        self.assertOutput('vtools select variant "testThousandGenomes.chr is not NULL" -c', '146\n')

    def testNSFP(self):
        'Test variants in dbNSFP'
        self.assertSucc('vtools use ann/testNSFP.ann --files ann/testNSFP.zip')
        # see if YRI=10/118 is correctly extracted
        self.assertOutput('''vtools execute "SELECT YRI_alt_lc, YRI_total_lc FROM testNSFP.testNSFP WHERE hg18pos=898186 AND alt='A';"''', '10\t118\n')
        self.assertOutput('''vtools execute "SELECT YRI_alt_lc, YRI_total_lc FROM testNSFP.testNSFP WHERE hg18pos=897662 AND alt='C';"''', '9\t118\n')
        
if __name__ == '__main__':
    unittest.main()
