function(expect_fail SRC)
    execute_process(
        COMMAND ${CMAKE_CXX_COMPILER} -std=c++20 -I${CMAKE_SOURCE_DIR}/include -c ${SRC} -o ${CMAKE_BINARY_DIR}/fail_test.o
        RESULT_VARIABLE RESULT
        OUTPUT_QUIET ERROR_QUIET
    )
    if(RESULT EQUAL 0)
        message(FATAL_ERROR "[G1] Expected compile fail, but succeeded: ${SRC}")
    else()
        message(STATUS "[G1] Correctly failed: ${SRC}")
    endif()
endfunction()

expect_fail(${CMAKE_SOURCE_DIR}/tests/compile_fail/fail_cross_dimension.cpp)
expect_fail(${CMAKE_SOURCE_DIR}/tests/compile_fail/fail_default_surprise.cpp)
expect_fail(${CMAKE_SOURCE_DIR}/tests/compile_fail/fail_ring_not_pow2.cpp)
