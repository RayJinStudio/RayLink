file(GLOB_RECURSE interface_srcs Interface/*.c)
file(GLOB_RECURSE usbd_srcs USBDevice/*.cpp)
file(GLOB_RECURSE hal_srcs Hal/*.cpp)

idf_component_register(SRCS 
                            "main.cpp"
                            ${interface_srcs}
                            ${usbd_srcs}
                            ${hal_srcs}
                        INCLUDE_DIRS 
                            "Interface"
                            "USBDevice"
                            "Hal"
                    )