add_library(arqnet
  sn_network.cpp
  bt_serialize.cpp)

target_link_libraries(arqnet PRIVATE libzmq)