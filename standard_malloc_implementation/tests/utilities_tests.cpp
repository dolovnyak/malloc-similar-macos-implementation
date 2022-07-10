#include <gtest/gtest.h>

#include <tuple>

extern "C" {
#include "malloc_internal.h"
#include "utilities.h"
}

template<class T>
using t_get_func_ptr = T(*)(const BYTE*);

template<class T>
using t_set_func_ptr = void (*)(BYTE*, T);

template<class T>
struct TestStruct {
    std::vector<T> test_values;
    std::vector<t_get_func_ptr<T>> get_value_func_list;
    std::vector<t_set_func_ptr<T>> set_value_func_list;
    std::vector<std::string> func_name_list;
};

template<class T>
struct Node_Header_Operations_Test : public ::testing::Test {
    static TestStruct<T> test_struct;
};

TYPED_TEST_SUITE_P(Node_Header_Operations_Test);

TYPED_TEST_P(Node_Header_Operations_Test, Check_All) {
    using TestClass = Node_Header_Operations_Test<TypeParam>;

    BYTE node_header[16];

    for (size_t i = 0; i < TestClass::test_struct.get_value_func_list.size(); ++i) {
        bzero(node_header, sizeof(BYTE) * 16);

        auto ptr_get_func = TestClass::test_struct.get_value_func_list[i];
        auto ptr_set_func = TestClass::test_struct.set_value_func_list[i];

        ASSERT_EQ(ptr_get_func(node_header), (TypeParam)0) << "Func type: " << TestClass::test_struct.func_name_list[i];

        for (auto value: TestClass::test_struct.test_values) {
            ptr_set_func(node_header, value);
            ASSERT_EQ(ptr_get_func(node_header), value) << "Func type: " << TestClass::test_struct.func_name_list[i];
        }
    }
}

REGISTER_TYPED_TEST_SUITE_P(Node_Header_Operations_Test, Check_All);

typedef ::testing::Types<uint64_t, BOOL, t_allocation_type> Types;
INSTANTIATE_TYPED_TEST_SUITE_P(Node_Header_Operations, Node_Header_Operations_Test, Types);

template<> TestStruct<uint64_t> Node_Header_Operations_Test<uint64_t>::test_struct{
        {1,             16,                  128,                  0xFF00FF,                   16777215},
        {get_prev_node_size, get_node_zone_start_offset, get_prev_free_node_zone_start_offset, get_next_free_node_zone_start_offset},
        {set_prev_node_size, set_node_zone_start_offset, set_prev_free_node_zone_start_offset, set_next_free_node_zone_start_offset},
        {"previous_node_size","node_zone_start_offset",   "prev_free_node_zone_start_offset", "next_free_node_zone_start_offset"}
};
template<> TestStruct<BOOL> Node_Header_Operations_Test<BOOL>::test_struct{
        {TRUE, FALSE, TRUE, TRUE},
        {get_node_available},
        {set_node_available},
        {"node_available"}};
template<> TestStruct<t_allocation_type> Node_Header_Operations_Test<t_allocation_type>::test_struct{
        {Tiny, Small, Large, Tiny},
        {get_node_allocation_type},
        {set_node_allocation_type},
        {"node_allocation_type"}
};

TEST(List_Operations, Zone_List) {
    t_zone zone1, zone2, zone3;
    t_zone* zone_list_start = nullptr;
    t_zone* zone_list_end = nullptr;

    add_zone_to_list(&zone_list_start, &zone_list_end, &zone1);
    ASSERT_EQ(zone_list_start, &zone1);
    ASSERT_EQ(zone_list_end, &zone1);
    ASSERT_EQ(zone1.prev, nullptr);
    ASSERT_EQ(zone1.next, nullptr);

    add_zone_to_list(&zone_list_start, &zone_list_end, &zone2);
    ASSERT_EQ(zone_list_start, &zone1);
    ASSERT_EQ(zone_list_end, &zone2);
    ASSERT_EQ(zone1.prev, nullptr);
    ASSERT_EQ(zone1.next, &zone2);
    ASSERT_EQ(zone2.prev, &zone1);
    ASSERT_EQ(zone2.next, nullptr);

    add_zone_to_list(&zone_list_start, &zone_list_end, &zone3);
    ASSERT_EQ(zone_list_start, &zone1);
    ASSERT_EQ(zone_list_end, &zone3);
    ASSERT_EQ(zone1.prev, nullptr);
    ASSERT_EQ(zone1.next, &zone2);
    ASSERT_EQ(zone2.prev, &zone1);
    ASSERT_EQ(zone2.next, &zone3);
    ASSERT_EQ(zone3.prev, &zone2);
    ASSERT_EQ(zone3.next, nullptr);

    delete_zone_from_list(&zone_list_start, &zone_list_end, &zone2);
    ASSERT_EQ(zone_list_start, &zone1);
    ASSERT_EQ(zone_list_end, &zone3);
    ASSERT_EQ(zone1.prev, nullptr);
    ASSERT_EQ(zone1.next, &zone3);
    ASSERT_EQ(zone3.prev, &zone1);
    ASSERT_EQ(zone3.next, nullptr);

    delete_zone_from_list(&zone_list_start, &zone_list_end, &zone1);
    ASSERT_EQ(zone_list_start, &zone3);
    ASSERT_EQ(zone_list_end, &zone3);
    ASSERT_EQ(zone3.prev, nullptr);
    ASSERT_EQ(zone3.next, nullptr);

    delete_zone_from_list(&zone_list_start, &zone_list_end, &zone3);
    ASSERT_EQ(zone_list_start, nullptr);
    ASSERT_EQ(zone_list_end, nullptr);
}