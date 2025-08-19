# Include from git submodule
idf_component_register(SRCS "src/SLIPStreamDecoder.cpp" "src/SLIPStreamEncoder.cpp" "src/SLIPBuffer.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES driver)
