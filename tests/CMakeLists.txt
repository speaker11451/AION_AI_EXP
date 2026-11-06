find_package(Threads REQUIRED)

# 关键：所有测试必须保留 assert 语义，否则 Release 下 -DNDEBUG 会让 assert 变成空操作
# 生产库（aion_m1 INTERFACE）不受影响，仍走默认 Release 行为
function(add_aion_test target src)
    add_executable(${target} ${src})
    target_link_libraries(${target} PRIVATE aion_m1 Threads::Threads)
    target_compile_options(${target} PRIVATE -UNDEBUG)
    add_test(NAME ${target} COMMAND ${target})
    set_tests_properties(${target} PROPERTIES TIMEOUT 15)
endfunction()

add_aion_test(contract_l0_l1    contract/l0_l1_compile.cpp)
add_aion_test(unit_l2           contract/unit_l2.cpp)
add_aion_test(unit_vc           contract/unit_vc.cpp)
add_aion_test(unit_spsc_mpsc    contract/unit_spsc_mpsc.cpp)
add_aion_test(unit_seqlock      contract/unit_seqlock.cpp)
add_aion_test(unit_compact_snapshot atomic/unit_compact_snapshot.cpp)
add_aion_test(unit_epoch_pool   atomic/unit_epoch_pool.cpp)
add_aion_test(unit_wait_graph_deadlock contract/unit_wait_graph_deadlock.cpp)
add_aion_test(g2_timewheel      stress/g2_timewheel.cpp)

add_aion_test(test_store_compact_torn_write atomic/test_store_compact_torn_write.cpp)
add_aion_test(test_burst_quota_fairness stress/test_burst_quota_fairness.cpp)
add_aion_test(test_wheel_overflow_ordering stress/test_wheel_overflow_ordering.cpp)
add_aion_test(test_seqlock_consistency contract/test_seqlock_consistency.cpp)
target_compile_definitions(test_seqlock_consistency PRIVATE AION_TEST_ACCESS)


# G1 compile-fail 矩阵
add_test(NAME g1_compile_fail
    COMMAND ${CMAKE_COMMAND}
    -DCMAKE_SOURCE_DIR=${CMAKE_SOURCE_DIR}
    -DCMAKE_CXX_COMPILER=${CMAKE_CXX_COMPILER}
    -P ${CMAKE_SOURCE_DIR}/tests/contract/check_compile_fail.cmake)
set_tests_properties(g1_compile_fail PROPERTIES TIMEOUT 10)
