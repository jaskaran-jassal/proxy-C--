// libraries used 
#include <iostream>
#include <cstring>
#include <unistd.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#define PORT 80 // port used for web server 
#define proxy_port 8080 // port used for the proxy

/**
* function for modifying "frog" to "Fred" all occurances
* htmlContent:- char pointer to store the content of html page
* contentlenght:- store bytes of content lenght in buffer 
*/
void modifyHtmlContent(char* htmlContent, size_t contentLength) {
    bool insideTag = false;                     // boolean flag
    for (size_t i = 0; i < contentLength; ++i) {        // loop over all the content length 
        if (htmlContent[i] == '<') {                    // if for html content "<" is found this refers the start of html content
            insideTag = true;                           // set flag to true
        } else if (htmlContent[i] == '>') {             // now if ">" is found refers end of html tag
            insideTag = false;                          // set flag to false
        } else if (!insideTag && strncasecmp(&htmlContent[i], "frog", 4) == 0) {    // if flad was true html content conatin 'f''r''o''g' is true
            strncpy(&htmlContent[i], "Fred", 4);                                    // replace html content with "Fred"
        }
    }
}
/*
*Function to replace images with image of a frog
*htmlContent:- char pointer to store the content of html page
*Contentlenght:- store bytes of content lenght in buffer 
*/
void replaceImagesWithCustomImage(char* htmlContent, size_t contentLength) {
    std::string content(htmlContent, contentLength);                   // initialize content with htmlcontent and lenght from parameter
    size_t pos = 0;                                                     // track position of content initially 0
    const std::string imageTagStart = "<img";                           // "<img" find from content of html
    const std::string imageTagEnd = ">";                                // find ">" from content
    
    // loop untill "<img" is found in html content
    while ((pos = content.find(imageTagStart, pos)) != std::string::npos) {
        size_t srcPos = content.find("src=", pos);      // find "src=" in html content and store in srcpos
        if (srcPos != std::string::npos) {              // if src= found in content
            // Find the position of the closing '>' of the image tag
            size_t endPos = content.find(imageTagEnd, srcPos);

            // Extract the src attribute value
            size_t srcValueStart = srcPos + 5;  // Start after 'src='
            size_t srcValueEnd = content.find_first_of("\"'", srcValueStart); // find '' or "" after src=
            std::string srcValue = content.substr(srcValueStart, srcValueEnd - srcValueStart);          // extract out the image link from html content

            // Check if the image is a PNG                          
            if (srcValue.find(".png") == std::string::npos) {           // if the the extracted srcvalue doesn't contain ".png" 
                // Replace the src attribute and add width
                content.replace(srcPos, endPos - srcPos, "src='https://t.ly/Te0tq' width='400'");
            }
            
            pos = endPos + imageTagEnd.length();                // proceed to the end of image tag if replaced
        } else {
            break;  // No src attribute found for image tag
        }
    }
    
    // Update the original HTML content with the modified content
    strncpy(htmlContent, content.c_str(), contentLength);
}

void proxy_webServer_socket(int client_socket){
    char buffer[5000] = {0};                        // buffer of size 5000
    int reading = recv(client_socket, buffer, 5000, 0);         // receive the content of client socket in reading
    if (reading < 0){
        perror("error reading from client");
        exit(EXIT_FAILURE);
    }
    std::cout << "Received request from client\n" << std::endl;                 // if reveived print on console
    std::cout << buffer <<std::endl;  
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);                        // create a server socket serving as a web server 
    if (server_socket < 0) {
        perror("Error creating server socket");
        exit(EXIT_FAILURE);
    }
    // details for address of web server
    struct sockaddr_in server_address;
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);

    struct hostent *address;    
    address = gethostbyname("pages.cpsc.ucalgary.ca") ; // get ip address of this particular web address
    bcopy((char *) address->h_addr, (char *) &server_address.sin_addr.s_addr, address->h_length);
    // connect server socket with proxy
    if (connect(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Error connecting to server");
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    // if connection was done print on console
    std::cout << "REQUEST TO SERVER" <<std::endl;   
    send(server_socket, buffer, 5000, 0);           // send the buffer to server socket reveived by the proxy

    int received = recv(server_socket, buffer, 5000, 0);            // store buffer received by the server socket in received
    if (received < 0) {
        perror("error receiving from server");
        close(client_socket);
        close(server_socket);
        exit(EXIT_FAILURE);
    }
    // 
    char* html_start = strstr(buffer, "<html");

    if (html_start != nullptr) {
        modifyHtmlContent(html_start, received - (html_start - buffer));            // replace frog with fred function
        replaceImagesWithCustomImage(buffer, received);             // replace image function called to replace new iamge 
        // send the new buffer back to browser
        send(client_socket, buffer, received, 0);
        std::cout << buffer <<std::endl;  
        std::cout << "RESPONSE AFTER REQUEST" <<std::endl;

    } else {
        // send the new buffer back to browser
        send(client_socket, buffer, received, 0);  
        std::cout << buffer <<std::endl;   

    }
    close(server_socket);   // close the server_socket
    close(client_socket);   // close the client_socket
}
/*
* main function to run the proxy 
*here server socket is serving as a web proxy socket  
*/
int main() {
    int server_socket, new_socket;              // creating server socket and new socket for every seesion 
    struct sockaddr_in address;                 // 
    int addrlen = sizeof(address);

    // creating a server socket serving as a server to web browser however a proxy server 
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    // proxy address details 
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = inet_addr("127.0.0.1");       // set up locally 
    address.sin_port = htons(proxy_port);                   // at port 8080
    // bind the server socket
    if (bind(server_socket, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    // listen for connection by server socket
    if (listen(server_socket, 10) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    std::cout << "Server is listening on port 8080..." << std::endl;    // print on console if server is listening 

    //while connection is true
    while (true) {
        // accept connection as new socket each time
        new_socket = accept(server_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            perror("error in accept");
            exit(EXIT_FAILURE);
        } else {
            // if connection is accepted 
            std::cout << "accepted connection successfully from browser." << std::endl;             
            proxy_webServer_socket(new_socket); // call proxy_wbserver_socket function with new_socket as parameter
            std::cout << "force reload to start a new session or press ^C to exit" << std::endl;    // message when a session is over
        }
    }
    return 0;                       // finish 
}