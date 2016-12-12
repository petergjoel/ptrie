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

#define BOOST_TEST_MODULE BinaryWrapper

#include <boost/test/unit_test.hpp>
#include <binarywrapper.h>

using namespace ptrie;

/*BOOST_AUTO_TEST_CASE(PassTest)
{
    BOOST_CHECK_EQUAL(true, true);
}*/

BOOST_AUTO_TEST_CASE(SimpleConstructorTest)
{
    binarywrapper_t basic;
    BOOST_CHECK_EQUAL(basic.size(), 0);
    basic.release();

    size_t b = 0;
    for(size_t i = 0; i < 128; ++i)
    {
        if((i % 8) == 1) ++b;
        binarywrapper_t sized(i);
        BOOST_CHECK_EQUAL(sized.size(), b);
        sized.release();
        BOOST_CHECK_EQUAL(sized.size(), 0);
    }

}

