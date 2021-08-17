/*
 * vectorTest.cpp
 *
 *  Created on: 2021-8-18
 *      Author: mzh
 */

#include "Vector.h"
#include "gtest/gtest.h"

namespace test
{
namespace vector_test
{

struct VectorTest : testing::Test
{

};

TEST_F(VectorTest, test_default_constructor)
{
    mystl::vector<int> v1;
    ASSERT_EQ(16, v1.capacity());
    ASSERT_EQ(0, v1.size());
}

}
}

