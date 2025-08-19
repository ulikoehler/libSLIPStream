# Include from git submodule
idf_component_register(SRCS "src/SLIPStream.cpp" "src/SLIPBuffer.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES driver)
