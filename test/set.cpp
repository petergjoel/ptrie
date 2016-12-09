/* 
 * Copyright (C) 2016  Peter G. Jensen <root@petergjoel.dk>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
// Created by Peter G. Jensen on 12/9/16.
#define BOOST_TEST_MODULE PTrieInsertion
#include <boost/test/unit_test.hpp>

#include <ptrie.h>
using namespace ptrie;

BOOST_AUTO_TEST_CASE(SizeTInsert)
{
    set<> set;

    for(size_t i = 0; i < 1000000; ++i)
    {
        auto exists = set.exists((uchar *) &i, sizeof(size_t));
        BOOST_CHECK_EQUAL(exists.first, false);
        auto inserted = set.insert((uchar*)&i, sizeof(size_t));
        BOOST_CHECK_EQUAL(inserted.first, true);
    }

    for(size_t i = 0; i < 1000000; ++i) {
        auto exists = set.exists((uchar *) &i, sizeof(size_t));
        BOOST_CHECK_EQUAL(exists.first, true);
    }
}

