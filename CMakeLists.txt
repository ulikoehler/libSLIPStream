# Include from git submodule
idf_component_register(SRCS "src/Decoder.cpp" "src/Encoder.cpp" "src/Buffer.cpp"
                    INCLUDE_DIRS "include"
                    REQUIRES driver)
