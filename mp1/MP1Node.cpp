/**********************************
 * FILE NAME: MP1Node.cpp
 *
 * DESCRIPTION: Membership protocol run by this Node.
 * 				Definition of MP1Node class functions.
 **********************************/

#include "MP1Node.h"

/*
 * Note: You can change/add any functions in MP1Node.{h,cpp}
 */

/**
 * Overloaded Constructor of the MP1Node class
 * You can add new members to the class if you think it
 * is necessary for your logic to work
 */
MP1Node::MP1Node(Member *member, Params *params, EmulNet *emul, Log *log, Address *address) {
	for( int i = 0; i < 6; i++ ) {
		NULLADDR[i] = 0;
	}
	this->memberNode = member;
	this->emulNet = emul;
	this->log = log;
	this->par = params;
	this->memberNode->addr = *address;
}

/**
 * Destructor of the MP1Node class
 */
MP1Node::~MP1Node() {}

/**
 * FUNCTION NAME: recvLoop
 *
 * DESCRIPTION: This function receives message from the network and pushes into the queue
 * 				This function is called by a node to receive messages currently waiting for it
 */
int MP1Node::recvLoop() {
    if ( memberNode->bFailed ) {
        memberNode->memberList.clear();
        return false;
    }
    else {
    	return emulNet->ENrecv(&(memberNode->addr), enqueueWrapper, NULL, 1, &(memberNode->mp1q));
    }
}

/**
 * FUNCTION NAME: enqueueWrapper
 *
 * DESCRIPTION: Enqueue the message from Emulnet into the queue
 */
int MP1Node::enqueueWrapper(void *env, char *buff, int size) {
	Queue q;
	return q.enqueue((queue<q_elt> *)env, (void *)buff, size);
}

/**
 * FUNCTION NAME: nodeStart
 *
 * DESCRIPTION: This function bootstraps the node
 * 				All initializations routines for a member.
 * 				Called by the application layer.
 */
void MP1Node::nodeStart(char *servaddrstr, short servport) {
    Address joinaddr;
    joinaddr = getJoinAddress();

    // Self booting routines
    if( initThisNode(&joinaddr) == -1 ) {
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "init_thisnode failed. Exit.");
#endif
        exit(1);
    }

    if( !introduceSelfToGroup(&joinaddr) ) {
        finishUpThisNode();
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Unable to join self to group. Exiting.");
#endif
        exit(1);
    }

    return;
}

/**
 * FUNCTION NAME: initThisNode
 *
 * DESCRIPTION: Find out who I am and start up
 */
int MP1Node::initThisNode(Address *joinaddr) {
	/*
	 * This function is partially implemented and may require changes
	 */
	int id = (int)(memberNode->addr.addr[0]);
	int port = (short)(memberNode->addr.addr[4]);
    
    memberNode->bFailed = false;
	memberNode->inited = true;
	memberNode->inGroup = false;
	
    // node is up!
	memberNode->nnb = 0;
	memberNode->heartbeat = 0;
	memberNode->pingCounter = TFAIL;
	memberNode->timeOutCounter = -1;
    
    initMemberListTable(memberNode);
    
    //MemberListEntry* member = new MemberListEntry(id, port, 1, this->par->getcurrtime());
    //memberNode->memberList.push_back(*member);
    //delete member;

    return 0;
}

/**
 * FUNCTION NAME: introduceSelfToGroup
 *
 * DESCRIPTION: Join the distributed system
 */
int MP1Node::introduceSelfToGroup(Address *joinaddr) {
	
	MessageHdr *msg;
#ifdef DEBUGLOG
    static char s[1024];
#endif

    if ( 0 == memcmp((char *)&(memberNode->addr.addr), (char *)&(joinaddr->addr), sizeof(memberNode->addr.addr))) {
        // I am the group booter (first process to join the group). Boot up the group
#ifdef DEBUGLOG
        log->LOG(&memberNode->addr, "Starting up group...");
        
#endif
        memberNode->inGroup = true;
    }
    else {
        size_t msgsize = sizeof(MessageHdr);
        msg = new MessageHdr();

        // create JOINREQ message: format of data is {struct Address myaddr}
        msg->msgType = JOINREQ;
        msg->myAddr = &memberNode->addr;
        
        //memcpy((char *)(msg+1), &memberNode->addr.addr, sizeof(memberNode->addr.addr));
        //memcpy((char *)(msg+1) + 1 + sizeof(memberNode->addr.addr), &memberNode->heartbeat, sizeof(long));
     
#ifdef DEBUGLOG
        sprintf(s, "Trying to join...");
        log->LOG(&memberNode->addr, s);
#endif

        // send JOINREQ message to introducer member
        emulNet->ENsend(&memberNode->addr, joinaddr, (char *)msg, msgsize);

        free(msg);
    }

    return 1;

}

/**
 * FUNCTION NAME: finishUpThisNode
 *
 * DESCRIPTION: Wind up this node and clean up state
 */
int MP1Node::finishUpThisNode(){
   /*
    * Your code goes here
    */
    
    initMemberListTable(memberNode);
    
    return 1;
}

/**
 * FUNCTION NAME: nodeLoop
 *
 * DESCRIPTION: Executed periodically at each member
 * 				Check your messages in queue and perform membership protocol duties
 */
void MP1Node::nodeLoop() {
    
    
    if (memberNode->bFailed) {
        memberNode->memberList.clear();
        return;
    }
    
    // Check my messages
    checkMessages();

    // Wait until you're in the group...
    if( !memberNode->inGroup ) {
        return;
    }

    // ...then jump in and share your responsibilites!
    nodeLoopOps();
    
    return;
}

/**
 * FUNCTION NAME: checkMessages
 *
 * DESCRIPTION: Check messages in the queue and call the respective message handler
 */
void MP1Node::checkMessages() {
    
    void *ptr;
    int size;

    // Pop waiting messages from memberNode's mp1q
    while ( !memberNode->mp1q.empty() ) {
        ptr = memberNode->mp1q.front().elt;
    	size = memberNode->mp1q.front().size;
    	memberNode->mp1q.pop();
    	recvCallBack((void *)memberNode, (char *)ptr, size);
    }
    return;
}

/**
 * FUNCTION NAME: recvCallBack
 *
 * DESCRIPTION: Message handler for different message types
 */
bool MP1Node::recvCallBack(void *env, char *data, int size ) {
	/*
	 * Your code goes here
	 */
	 
	MessageHdr* msg = (MessageHdr*) data;
	 	 
	if ( (unsigned)size < sizeof(MessageHdr)) 
	{
        return false;
    }

    int mem_id = (int)(msg->myAddr->addr[0]);
    short mem_port = (short)(msg->myAddr->addr[4]);
    
    int id = (int)(memberNode->addr.addr[0]);
	int port = (short)(memberNode->addr.addr[4]);
    
    if (mem_id == id && mem_port == port)
        return false;
    
    long timestamp = this->par->getcurrtime();
    
    MemberListEntry* member = isPresentInList(mem_id, mem_port);
    
    if(msg->msgType == JOINREQ){
    
        memberNode->heartbeat += 1;
    
        //printMsg(msg);
        
        if (member == nullptr) {
            long heartbeat = 1;    
            member = new MemberListEntry(mem_id, mem_port, heartbeat, timestamp);
            memberNode->memberList.push_back(*member);
            log->logNodeAdd(&memberNode->addr ,makeAddress(mem_id, mem_port));
        }
        if (member != nullptr) {
            member->heartbeat += 1;
            member->timestamp = timestamp;
        }
        
        MessageHdr* new_msg = new MessageHdr();
        new_msg->msgType = JOINREP;
        new_msg->myAddr = &memberNode->addr;

        int listsize = memberNode->memberList.size();
        
        for (int i = 0; i < listsize; i ++) {
            MemberListEntry mem = memberNode->memberList[i];
            member = new MemberListEntry(mem.id, mem.port, mem.heartbeat, this->par->getcurrtime());
            new_msg->myMembers.push_back(*member);            
        }
        
        member = new MemberListEntry(id, port, memberNode->heartbeat, this->par->getcurrtime());
        new_msg->myMembers.push_back(*member);
        
        emulNet->ENsend( &memberNode->addr, msg->myAddr, (char*)new_msg, sizeof(MessageHdr));
        
        //delete new_msg;
    }
    else if(msg->msgType == JOINREP){
    
        //printMsg(msg);   
        
        if (member == nullptr ) {
            memberNode->inGroup = true;
            member = new MemberListEntry(mem_id, mem_port, 1, timestamp);
            memberNode->memberList.push_back(*member);
            log->logNodeAdd(&memberNode->addr ,makeAddress(mem_id, mem_port));
        }
        /*
        else {
            member->heartbeat += 1;
        }
        */
        
        if (msg->myMembers.size()> 0) {
            updateMemberListTable(msg);
        }           
             
    }
    else if(msg->msgType == GOSSIP && memberNode->memberList.size()>0 && memberNode->inGroup){
        
        //printMsg(msg);
        
        updateMemberListTable(msg);          
    }
    
    //delete msg;
    
    //printMyList();
    return true;
    
}

void MP1Node::updateMemberListTable(MessageHdr* msg)
{
    if (!memberNode->inGroup) {
        return;
    }
    
    int id = (int)(memberNode->addr.addr[0]);
	int port = (short)(memberNode->addr.addr[4]);
	
	int mem_id = (int)(msg->myAddr->addr[0]);
    short mem_port = (short)(msg->myAddr->addr[4]);
    
	MemberListEntry* sender = isPresentInList(mem_id, mem_port);
	if (sender == nullptr) {
        sender = new MemberListEntry(id, port, 1 , this->par->getcurrtime());
        memberNode->memberList.push_back(*sender);
        log->logNodeAdd(&memberNode->addr ,makeAddress(mem_id, mem_port));
    }
    else
        sender->heartbeat += 1;
    //delete sender;
	
    for (int i = 0; i < msg->myMembers.size() ; i++) {
        MemberListEntry* entry = &msg->myMembers[i];
        MemberListEntry* member = isPresentInList(entry->id, entry->port);

        if(entry->id ==id && entry->port == port) {
            if (memberNode->heartbeat <= entry->heartbeat)
                memberNode->heartbeat = entry->heartbeat + 1;
            continue;
        }
        
        int listsize = memberNode->memberList.size();
        
        if (member != nullptr) {
            for (int i = 0; i < memberNode->memberList.size(); i ++) {
                MemberListEntry* mem = &memberNode->memberList[i];
                if (mem->id == entry->id && mem->port == entry->port) {
                    if (mem->heartbeat < entry->heartbeat) {
                        mem->heartbeat = entry->heartbeat ;
                        mem->timestamp = par->getcurrtime();
                    }    
                }
                else if (mem->id == mem_id && mem->port == mem_port) {
                    mem->timestamp = par->getcurrtime();
                }
            }

            
        }    
        else {
            member = new MemberListEntry(entry->id, entry->port, entry->heartbeat, this->par->getcurrtime());
            memberNode->memberList.push_back(*member); 
            log->logNodeAdd(&memberNode->addr ,makeAddress(entry->id, entry->port));           
        }
        
    }
    
       
}

void MP1Node::printMyList() {
    int listsize = memberNode->memberList.size();
        
    MessageHdr* new_msg = new MessageHdr();
    new_msg->msgType = TEST;
    new_msg->myAddr = &memberNode->addr;

    if ((int)memberNode->addr.addr[0] != 0) {
    for (int i = 0; i < listsize; i ++) {
        MemberListEntry mem = memberNode->memberList[i];
        MemberListEntry* member = new MemberListEntry(mem.id, mem.port, mem.heartbeat, mem.timestamp);
        new_msg->myMembers.push_back(*member);            
    }
    printMsg(new_msg);
    delete new_msg;
    }
        
    
    
}

MemberListEntry* MP1Node::isPresentInList(int id, short port)
{
    int i;
    MemberListEntry* member;

    for (i = 0; i < memberNode->memberList.size(); i++) {
        member = memberNode->memberList.data() + i;
        if (member->id == id && member->port == port) {
            break;
        }
    }
    
    if (i < memberNode->memberList.size())         
        return member;
    
    return nullptr;
}

void MP1Node::printMsg(MessageHdr* msg)
{
    cout << memberNode->addr.getAddress() << " Got message from " << msg->myAddr->getAddress() << " Type: " << msg->msgType << " List: ";
    
    for (int i = 0; i < msg->myMembers.size() && i < 10; i++) {
        cout << msg->myMembers[i].id << ":" <<msg->myMembers[i].port << "[" << msg->myMembers[i].heartbeat << "][" << msg->myMembers[i].timestamp << "] ";    
    }
    cout << "END " << memberNode->heartbeat  << "[" << par->getcurrtime() << "]" << endl;
}

Address* MP1Node::makeAddress(int id, short port) 
{
    Address* address = new Address();
    memcpy(&address->addr[0], &id, sizeof(int));
    memcpy(&address->addr[4], &port, sizeof(short));
    return address;   
} 

/**
 * FUNCTION NAME: nodeLoopOps
 *
 * DESCRIPTION: Check if any node hasn't responded within a timeout period and then delete
 * 				the nodes
 * 				Propagate your membership list
 */
void MP1Node::nodeLoopOps() {

	/*
	 * Your code goes here
	 */
	 
	memberNode->heartbeat += 1;
    
    
    int id = (int)(memberNode->addr.addr[0]);
	int port = (short)(memberNode->addr.addr[4]);
	
	/*
	MemberListEntry* member = isPresentInList(id, port);
	 if (member == nullptr && !memberNode->inGroup) {
         member = new MemberListEntry(id, port, memberNode->heartbeat, this->par->getcurrtime());
         memberNode->memberList.push_back(*member);
     }
     else 
        member->heartbeat = memberNode->heartbeat; 
    */
    
    
    //printMyList();

    int j = memberNode->memberList.size();
    for (int i = 0; i < j; i++) {
        MemberListEntry* member = memberNode->memberList.data() + i;;
        if (makeAddress(id,port) == makeAddress(member->id,member->port)) {
            member->heartbeat = memberNode->heartbeat;
        }
        else 
        if (par->getcurrtime() - member->timestamp >= TREMOVE && member->id != id) {
            log->logNodeRemove(&memberNode->addr, makeAddress(member->id, member->port));
            memberNode->memberList.erase(memberNode->memberList.begin() + i);
            i-=1;
            j = memberNode->memberList.size();
        }
    }
    
    //printMyList();

    MessageHdr* msg = new MessageHdr();
    msg->msgType = GOSSIP;
    msg->myAddr = &memberNode->addr;
    
    int listsize = memberNode->memberList.size();
        
    for (int i = 0; i < listsize; i ++) {
        MemberListEntry mem = memberNode->memberList[i];
        MemberListEntry* member = new MemberListEntry(mem.id, mem.port, mem.heartbeat, mem.timestamp);
        msg->myMembers.push_back(*member);  
        //delete member;
    }
    MemberListEntry* member = new MemberListEntry(id, port, memberNode->heartbeat, this->par->getcurrtime());
    msg->myMembers.push_back(*member);  
    //msg->myMembers = memberNode->memberList;
    
    for (MemberListEntry entry : memberNode->memberList) {
        Address* addr = makeAddress(entry.getid(), entry.getport());
        emulNet->ENsend(&memberNode->addr, addr, (char*)msg, sizeof(MessageHdr));
        delete addr;
    }
    //delete msg;//Do No Delete
    
    //printMyList();
    
    return;
}

/**
 * FUNCTION NAME: isNullAddress
 *
 * DESCRIPTION: Function checks if the address is NULL
 */
int MP1Node::isNullAddress(Address *addr) {
	return (memcmp(addr->addr, NULLADDR, 6) == 0 ? 1 : 0);
}

/**
 * FUNCTION NAME: getJoinAddress
 *
 * DESCRIPTION: Returns the Address of the coordinator
 */
Address MP1Node::getJoinAddress() {
    Address joinaddr;

    memset(&joinaddr, 0, sizeof(Address));
    *(int *)(&joinaddr.addr) = 1;
    *(short *)(&joinaddr.addr[4]) = 0;

    return joinaddr;
}

/**
 * FUNCTION NAME: initMemberListTable
 *
 * DESCRIPTION: Initialize the membership list
 */
void MP1Node::initMemberListTable(Member *memberNode) {
	memberNode->memberList.clear();
}

/**
 * FUNCTION NAME: printAddress
 *
 * DESCRIPTION: Print the Address
 */
void MP1Node::printAddress(Address *addr)
{
    printf("%d.%d.%d.%d:%d \n",  addr->addr[0],addr->addr[1],addr->addr[2],
                                                       addr->addr[3], *(short*)&addr->addr[4]) ;    
}
