# A C++ implementation of Rign Based membership protocol with Failure detection

The protocol satisfies:
1> Completeness all the time: every non-faulty process must detect every node join, failure, and leave, 
2> Accuracy of failure detection when there are no message losses and message delays are small. When there are message losses, completeness must be satisfied and accuracy must be high. It must achieve all of these even under simultaneous multiple failures.

Heartbeating is implemented with a gossip-style messaging. 

## Layers

The three-layer implementation framework provides multiple parallel execution of peers within one process running a single-threaded simulation engine. 
Here is how the three layers work:

### Layer1: Emulated Network: EmulNet
EmulNet provides the following functions::
- void *ENinit(Address *myaddr, short port);
- int ENsend(Address *myaddr, struct address *addr, string data);
- int ENrecv(Address *myaddr, int (* enqueue)(void *, char *, int), struct timeval *t, int times, void *queue);
- int ENcleanup();

### Layer2: Application
This layer drives the simulation. Files Application.{cpp,h} contain code for this. The main() function runs in synchronous periods (globaltime variable).

### Layer2: P2P
The functionality for this layer is pretty limited at this time. Files MP1Node.{cpp,h} contain code for this. This is the layer responsible for implementing the membership protocol.



