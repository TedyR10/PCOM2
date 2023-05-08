**Name: Theodor-Ioan Rolea**

**Group: 323CA**

# HW2 PCOM

### Description
* This project focuses on creating a client-server app for managing subscriptions over TCP & UDP.  

***

### Program Structure
* The program is split into 2 parts, the subscriber part and the server part. Additionally, helpers.h contains the tcp, udp, packet & client structures used.

***

### How the program works

#### Server

We initialize the TCP and UDP sockets, then the server, and add the sockets into the descriptors array.

Within the while loop, we use the poll function to control the descriptors.

We first check whether we have received a command from stdin, where the function handleSTDIN performs the background check for the "exit" command. If that is the case, the loop is broken, and the allocated resources are freed.

Then, we check whether any subscriber sockets have received messages. The response is handled inside the response_subscriber function where we receive the packet, identify the client that sent the packet, and determine the action the server needs to perform based on the packet type: 
- If the client wants to subscribe to a topic, we check if they are already subscribed. If not, we set the SF (Store-and-Forward) and add the topic to the subscriptions array.
- If the client wants to unsubscribe from a topic, we shift the elements of the subscriptions and SF arrays by one element.
- Finally, we check if the client has disconnected, delete their socket from the descriptor array, and close the connection.

Next, we check for a UDP response from our UDP clients. This is handled through the handleUDP function, where we receive the UDP packet and copy the necessary contents into a new TCP packet to send to the subscribers of the topic for which the UDP client made an update. If the subscribed clients are connected, they will receive the packet immediately; otherwise, it will be stored in the pending array and sent once they come back online.

Finally, we check for a TCP connection from TCP clients. We accept the connection, receive the client ID, and check if the client has previously logged in. If it's a new client, we add an entry to our clients array. Otherwise, we check if they are already connected on another socket. If they are reconnecting, we send them the pending packets, if any.

***

#### Subscriber
* Similar to the server, we initialize the sockets and the server address, and add the sockets to the descriptor array. We then use poll to control the descriptors.

If we receive input from STDIN, we determine the type of command the subscriber wants to perform:
- "exit" disconnects the subscriber and closes the connection with the server.
- "subscribe" subscribes the user to a new topic.
- "unsubscribe" unsubscribes the user from a topic.

Next, we check if we have received any message from the server and display it.

***

### Mentions and challenges
* I thoroughly enjoyed working on this assignment, despite the challenges with the checker. Each time I made a mistake, I had to reboot the VM because the checker wouldn't run again.