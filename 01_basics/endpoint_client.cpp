/* endpoint_client.cpp */
#include <boost/asio.hpp>
#include <iostream>
#include <string_view>
#include <cinttypes>
#include <boost/asio/ip/address.hpp>

int main()
{
    // Step 1. Assume that the client application has already
    // obtained the IP-address and the protocol port number.
    std::string_view raw_ip_address = "127.0.0.1";
    uint16_t port_num = 3333;

    // Used to store information about error that happens
    // while parsing the raw IP-address.
    boost::system::error_code ec;
    
    // Step 2. Using IP protocol version independent address
    // representation.
    boost::asio::ip::address ip_address = boost::asio::ip::make_address(raw_ip_address, ec);

    if (ec)
    {
        // Provided IP address is invalid. Breaking execution
        std::cerr << "Failed to parse the IP address. Error code = "
        << ec.value() << ". Message: " << ec.message() << '\n';
        return ec.value();
        
    }

    // Step 3.
    boost::asio::ip::tcp::endpoint ep(ip_address, port_num);

    // Step 4. The endpoint is ready and can be used to specify a
    // particular server in the network the client wants to
    // comunicate with.

    return 0;
}
