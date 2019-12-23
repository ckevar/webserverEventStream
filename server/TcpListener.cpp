#include "TcpListener.h"
#include <iostream>
#include <cstring>
#include <sstream>
#include <cstdio>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int TcpListener::init()
{
	// Create a socket
	m_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (m_socket < 0) {
		fprintf(stderr,"[ERROR:] cannot open socket");
	}

	// Bind the ip address and port to a socket
	struct sockaddr_in hint;
	hint.sin_family = AF_INET;
	hint.sin_port = htons(m_port);
	inet_pton(AF_INET, m_ipAddress, &hint.sin_addr);

	if (bind(m_socket, (sockaddr*)&hint, sizeof(hint)) < 0) {
		fprintf(stderr, "[ERROR:] at binding\n");
		return -1;
	}
	// Tell the socket is for listening 
	if (listen(m_socket, SOMAXCONN) < 0) {
		fprintf(stderr, "[ERROR:] at listening\n");
		return -1;
	} 

	// Create the master file descriptor set and assing -1 file descriptor, and no event
	for (int i = 0; i < (MAX_CLIENTS + 1); ++i) {
		m_master[i].fd = -1;
		m_master[i].events = 0;
		streamMe[i] = false;
	}

	// Add our first socket that we're interested in interacting with; the listening socket!
	// It's important that this socket is added for our server or else we won't 'hear' incoming
	// connections 
	available = MAX_CLIENTS;
	m_master[0].fd = m_socket; // the index zero is our listening socket

	running = true;	// Enable server to run
	return 0;
}

int TcpListener::run() {
	// this will be changed by the \quit command (see below, bonus not in video!)

	while (running) {
		std::cout << "[DEBUG:] Available seats " << available << " out of " << MAX_CLIENTS << std::endl;
		m_master[0].events = (available > 0) ? POLLIN : 0;	// Updating the event based on the availability
		
		// 30 miliseconds to wait in case there are clients, 
		// otherwise let's get stuck in poll until, some clients show up
		m_timeout = (available == MAX_CLIENTS) ? -1 : 30;
		// See who's talking to us
		int socketCount = poll(m_master, MAX_CLIENTS, m_timeout);	
		
		if (!socketCount) onTimeOut();
		// Is it an inbound communication?
		if (m_master[0].revents == POLLIN) {
			// Accept a new connection
			int client = accept(m_socket, nullptr, nullptr);

			// Add the new connection to the list of connected clients
			allocateClient(client);

			onClientConnected(client);
			socketCount--;
		}	

		int i = 1;
		// It's an inbound message
		// Loop through all the current connections / potential connect
		while (socketCount > 0) {

			if(m_master[i].revents == POLLIN) {
				char buf[4096];
				memset(buf, 0, 4096);
				int sock = m_master[i].fd;

				// Receive message
				int bytesIn = recv(sock, buf, 4096, 0);
				if (bytesIn <= 0) {
					// Drop the client
					onClientDisconnected(sock);
					close(sock);
					deallocateClient(sock);
				}
				else {
					onMessageReceived(sock, buf, bytesIn);
				}
				socketCount--;
			}
			i++;
		}
	}

	// Remove the listening socket from the master file descriptor set and close it
	// to prevent anyone else trying to connect.
	close(m_master[0].fd);

	int i = 1;
	while (available != MAX_CLIENTS) {

		if (m_master[i].fd) {
			// Get the socket number
			close(m_master[i].fd);
			// Remove it from the master file list and close the socket
			m_master[i].fd = -1;
			available++;
		}
		i++;
	}

	return 0;
}

void TcpListener::stop(){
	running = false;
}

void TcpListener::allocateClient(int client) {
	unsigned i = 1;

	while(m_master[i].fd > -1) i++;
	available--;

	m_master[i].fd = client;
	m_master[i].events = POLLIN;
}

// Handler to allocate when a client explicitly ask for streaming data
void TcpListener::allocateStreaming(int client) {
	unsigned i = 1;
	while(m_master[i].fd != client) i++;
	streamMe[i] = true;
}

void TcpListener::deallocateClient(int client) {
	unsigned i = 1; // starts at 1, because internal listener socket is at 0
	
	while(m_master[i].fd != client) i++;
	available++;
	
	m_master[i].fd = -1;
	m_master[i].events = 0;
	streamMe[i] = false;
}


void TcpListener::sendToClient(int clientSocket, const char* msg, int length) {
	send(clientSocket, msg, length, 0);
}

void TcpListener::broadcastToClients(int sendingClient, const char* msg, int length) {
	int j = available;
	int i = 1;			// start at 1, because the internal listener is at 0
	while (j > 0) {		// Send to all available devices
		int outSock = m_master[i].fd;
		if (outSock > -1) {	
			if (outSock != sendingClient) // dont sent to the one who's broadcasting
				sendToClient(outSock, msg, length);
			j--;
		}
		i++;
	}
}

void TcpListener::streamToClients(const char* msg, int length) {
	int j = available;
	int i = 1;			// start at 1, because the internal listener is at 0
	while (j > 0) {		// Send to all available devices
		int outSock = m_master[i].fd;
		if (outSock > -1) {	
			if (streamMe[i])
				sendToClient(outSock, msg, length);
			j--;
		}
		i++;
	}	
}

void TcpListener::onClientConnected(int clientSocket) {

}

void TcpListener::onClientDisconnected(int clientSocket) {

}

void TcpListener::onMessageReceived(int clientSocket, const char* msg, int length) {

}

void TcpListener::onTimeOut() {

}