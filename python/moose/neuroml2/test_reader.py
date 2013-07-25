# test_reader.py --- 
# 
# Filename: test_reader.py
# Description: 
# Author: 
# Maintainer: 
# Created: Wed Jul 24 16:02:21 2013 (+0530)
# Version: 
# Last-Updated: Wed Jul 24 17:35:37 2013 (+0530)
#           By: subha
#     Update #: 40
# URL: 
# Keywords: 
# Compatibility: 
# 
# 

# Commentary: 
# 
# 
# 
# 

# Change log:
# 
# 
# 
# 
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as
# published by the Free Software Foundation; either version 3, or
# (at your option) any later version.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; see the file COPYING.  If not, write to
# the Free Software Foundation, Inc., 51 Franklin Street, Fifth
# Floor, Boston, MA 02110-1301, USA.
# 
# 

# Code:

import unittest
from reader import NML2Reader

class TestReader(unittest.TestCase):
    def setUp(self):
        self.reader = NML2Reader()
        self.lib = moose.Neutral('/library')
        self.filename = 'test_files/Purk2M9s.nml'
        self.reader.read(self.filename)

    def test_basic_loading(self):
        self.assertEqual(self.reader.filename, self.filename, 'filename was not set')
        self.assertIsNotNone(self.reader.doc, 'doc is None')

    def test_createCellPrototype(self):
        cell = self.reader.createCellPrototype(0)
        print dir(cell)
        

if __name__ == '__main__':
    unittest.main()

# 
# test_reader.py ends here
