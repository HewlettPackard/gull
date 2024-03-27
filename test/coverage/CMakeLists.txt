set(COVERAGE_REMOVE_PATHS
    '/usr/*'
    '${PROJECT_SOURCE_DIR}/third-party/*'
)

set(COVERAGE_PATH ${PROJECT_BINARY_DIR}/test/coverage)
set(Coverage_NAME coverage)

#Add this target only for coverage build
if(CMAKE_BUILD_TYPE MATCHES Coverage)

# Check prereqs
find_program( GCOV_BIN gcov )
find_program( LCOV_BIN  lcov )
find_program( GENHTML_BIN genhtml )

if(NOT LCOV_BIN)
message(FATAL_ERROR "lcov not found! Aborting...")
endif() # NOT LCOV_BIN

if(NOT GENHTML_BIN)
message(FATAL_ERROR "genhtml not found! Aborting...")
endif() # NOT GENHTML_BIN


#cov-clean
add_custom_target(cov-clean
# Cleanup lcov
COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN} -directory ${PROJECT_BINARY_DIR}/src --zerocounters

# Clean the coverage directory
COMMAND rm -rf ${COVERAGE_PATH}/*

# Create baseline.
COMMAND ${LCOV_BIN}  --gcov-tool ${GCOV_BIN} -c -i -d ${PROJECT_BINARY_DIR}/src -o ${COVERAGE_PATH}/${Coverage_NAME}.base

COMMENT "Resetting code coverage counters to zero."
)

#cov-run-tests
add_custom_target(cov-run-tests
# Run tests
WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
COMMAND make test || echo "*** make test failed. ***"
)

#cov-report
add_custom_target(cov-report
# Capture coverage data.
COMMAND ${LCOV_BIN} --gcov-tool ${GCOV_BIN} --directory ${PROJECT_BINARY_DIR}/src --capture --output-file ${COVERAGE_PATH}/${Coverage_NAME}.info

# add with baseline data.
COMMAND ${LCOV_BIN} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_BIN} -a ${COVERAGE_PATH}/${Coverage_NAME}.base -a ${COVERAGE_PATH}/${Coverage_NAME}.info --output-file ${COVERAGE_PATH}/${Coverage_NAME}.total

# Generage final coverage info file afer removing coverage data of /usr/* and other third-party projects.
COMMAND ${LCOV_BIN} ${Coverage_LCOV_ARGS} --gcov-tool ${GCOV_BIN} --remove ${COVERAGE_PATH}/${Coverage_NAME}.total ${COVERAGE_REMOVE_PATHS} --output-file ${COVERAGE_PATH}/${Coverage_NAME}.info.final

# Generate HTML reports.
COMMAND ${GENHTML_BIN} ${Coverage_GENHTML_ARGS} -o ${COVERAGE_PATH} ${COVERAGE_PATH}/${Coverage_NAME}.info.final

# Display a short list of report on console.
COMMAND ${LCOV_BIN} -l ${COVERAGE_PATH}/${Coverage_NAME}.info.final

COMMAND echo "*** Lcov code coverage info report saved in ${COVERAGE_PATH}/${Coverage_NAME}.info ***"
COMMAND echo "*** Open ${COVERAGE_PATH}/index.html in your browser to view the coverage report ***"

WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
DEPENDS ${Coverage_DEPENDENCIES}
)

#coverage - Call all the targets to build the coverage
add_custom_target(coverage
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
    COMMAND make cov-clean
    COMMAND make cov-run-tests
    COMMAND make cov-report
)

endif()
