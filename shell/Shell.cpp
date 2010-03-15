/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include <pthread.h>
#include "header.h"
#include "Shell.h"
#include "Dinfo.h"

// Want to separate out this search path into the Makefile options
#include "../scheduling/Tick.h"
#include "../scheduling/TickPtr.h"
#include "../scheduling/Clock.h"

ProcInfo Shell::p_;

const BindIndex requestShellOp = 2;
const BindIndex ackShellOp = 1;
const BindIndex requestGetSlot = 0;

static BindIndex bi = 2;

static SrcFinfo4< string, Id, Id, string  > *requestCreate =
		new SrcFinfo4< string, Id, Id, string  >( "requestCreate",
			"requestCreate( class, parent, newElm, name ): "
			"creates a new Element on all nodes with the specified Id. "
			"Initiates a callback to indicate completion of operation. "
			"Goes to all nodes including self.", 
			bi++ );

static SrcFinfo0* ackCreate =
		new SrcFinfo0( "ackCreate",
			"ackCreate():"
			"Acknowledges receipt and completion of Create command."
			"Goes back only to master node.",
			bi++ );

static SrcFinfo1< Id  > *requestDelete =
		new SrcFinfo1< Id >( "requestDelete",
			"requestDelete( doomedElement ):"
			"Deletes specified Element on all nodes."
			"Initiates a callback to indicate completion of operation."
			"Goes to all nodes including self.", 
			bi++ );

static SrcFinfo0* ackDelete =
		new SrcFinfo0( "ackDelete",
			"ackDelete():"
			"Acknowledges receipt and completion of Delete command."
			"Goes back only to master node.",
			bi++ );

static DestFinfo create( "create", 
			"create( class, parent, newElm, name",
			new OpFunc4< Shell, string, Id, Id, string>( &Shell::create ) );

static DestFinfo del( "delete", 
			"Destroys Element, all its messages, and all its children. Args: Id",
			new OpFunc1< Shell, Id >( & Shell::destroy ) );

static DestFinfo handleAckCreate( "handleAckCreate", 
			"Keeps track of # of responders to ackCreate. Args: none",
			new OpFunc0< Shell >( & Shell::handleAckCreate ) );

static DestFinfo handleAckDelete( "handleAckCreate", 
			"Keeps track of # of responders to ackCreate. Args: none",
			new OpFunc0< Shell >( & Shell::handleAckCreate ) );

static Finfo* shellMaster[] = {
	requestCreate, &handleAckCreate, requestDelete, &handleAckDelete, };
static Finfo* shellWorker[] = {
	&create, ackCreate, &del, ackDelete };
/*
static SrcFinfo4< Id, string, Id, string  > *requestMsg =
		new SrcFinfo4< string, Id, Id, MsgId  >( "requestMsg",
			"requestMsg( msgtype, e1, e2, msgid ): "
			"creates a new Msg on all nodes with the specified MsgId. "
			"Initiates a callback to indicate completion of operation. "
			"Goes to all nodes including self."
			"This is a low-level call.", 
			requestShellOp );

static SrcFinfo0* ackMsg =
		new SrcFinfo0( "ackMsg",
			"ackMsg():"
			"Acknowledges receipt and completion of requestMsg command."
			"Goes back only to master node.",
			ackShellOp );
*/

static SrcFinfo1< FuncId > *requestGet = 
		new SrcFinfo1< FuncId >( "requestGet",
			"Function to request another Element for a value",
			requestGetSlot );

const Cinfo* Shell::initCinfo()
{
	/*
	static Finfo* reacFinfos[] = {
		new Finfo( setKf_ ),
		new Finfo( setKb_ ),
	};
	*/
	static Finfo* shellFinfos[] = {
		new ValueFinfo< Shell, string >( 
			"name",
			"Name of object", 
			&Shell::setName, 
			&Shell::getName ),

		new ValueFinfo< Shell, bool >( 
			"quit",
			"Flag to tell the system to quit", 
			&Shell::setQuit, 
			&Shell::getQuit ),
////////////////////////////////////////////////////////////////
// Dest Finfos: Functions handled by Shell
////////////////////////////////////////////////////////////////
		new DestFinfo( "handleGet", 
			"Function to handle returning values for 'get' calls.",
			new RetFunc< Shell >( &Shell::handleGet ) ),
		new DestFinfo( "start", 
			"Starts off a simulation for the specified run time, automatically partitioning among threads if the settings are right",
			new OpFunc1< Shell, double >( & Shell::start ) ),
		new DestFinfo( "setclock", 
			"Assigns clock ticks. Args: tick#, dt, stage",
			new OpFunc3< Shell, unsigned int, double, unsigned int >( & Shell::setclock ) ),
		new DestFinfo( "loadBalance", 
			"Set up load balancing",
			new OpFunc0< Shell >( & Shell::loadBalance ) ),

		new DestFinfo( "create", 
			"create( class, parent, newElm, name",
			new OpFunc4< Shell, string, Id, Id, string>( &Shell::create )),
		new DestFinfo( "delete", 
			"Destroys Element, all its messages, and all its children. Args: Id",
			new OpFunc1< Shell, Id >( & Shell::destroy ) ),

		new DestFinfo( "addmsg", 
			"Adds a Msg between specified Elements. Args: Src, Dest, srcField, destField",
			new OpFunc4< Shell, Id, Id, string, string >( & Shell::addmsg ) ),

////////////////////////////////////////////////////////////////
//  Predefined Msg Src and MsgDests.
////////////////////////////////////////////////////////////////

		requestGet,
		requestCreate,
		ackCreate,
		requestDelete,
		ackDelete,
////////////////////////////////////////////////////////////////
//  Shared msg
////////////////////////////////////////////////////////////////

		new SharedFinfo( "master",
			"Issues commands from master shell to worker shells located "
			"on different nodes. Also handles acknowledgements from them.",
			shellMaster, sizeof( shellMaster ) / sizeof( const Finfo* )
		),
		new SharedFinfo( "worker",
			"Handles commands arriving from master shell on node 0."
			"Sends out acknowledgements from them.",
			shellWorker, sizeof( shellWorker ) / sizeof( const Finfo* )
		),
	};

	static Cinfo shellCinfo (
		"Shell",
		0, // No base class.
		shellFinfos,
		sizeof( shellFinfos ) / sizeof( Finfo* ),
		new Dinfo< Shell >()
	);

	return &shellCinfo;
}

static const Cinfo* shellCinfo = Shell::initCinfo();


Shell::Shell()
	: name_( "" ),
		quit_( 0 ), 
		isSingleThreaded_( 0 ), numCores_( 1 ), numNodes_( 1 )
{
	;
}

////////////////////////////////////////////////////////////////
// Parser functions.
////////////////////////////////////////////////////////////////

/**
 * This is the version used by the parser. Acts as a blocking,
 * serial-like interface to a potentially multithread, multinode call.
 * Returns the new Id index.
 * The data of the new Element is not necessarily allocated at this point,
 * that can be deferred till the global Instantiate or Reset calls.
 * Idea is that the model should be fully defined before load balancing.
 *
 */
Id Shell::doCreate( string type, Id parent, string name, vector< unsigned int > dimensions )
{
	Id ret = Id::nextId();
	// Here we would do the 'send' on an internode msg to do the actual
	// Create.
	innerCreate( type, parent, ret, name );
	return ret;
}

bool Shell::doDelete( Id i )
{
	destroy( i );
	return 1;
}

MsgId Shell::doCreateMsg( Id src, const string& srcField, Id dest,
	const string& destField, const string& msgType )
{
	const Finfo* f1 = src()->cinfo()->findFinfo( srcField );
	if ( !f1 ) {
		cout << "add: Error: Failed to find field " << srcField << 
			" on src:\n"; // Put name here.
		return 0;
	}
	const Finfo* f2 = dest()->cinfo()->findFinfo( destField );
	if ( !f2 ) {
		cout << "add: Error: Failed to find field " << srcField << 
			" on src:\n"; // Put name here.
		return 0;
	}

	Msg* m = 0;
	if ( msgType == "OneToOneMsg" )
		m = new OneToOneMsg( src(), dest() );
	if ( msgType == "OneToAllMsg" )
		m = new OneToAllMsg( src.eref(), dest() );
	// And so on, lots of msg types here.
	
	if ( m ) {
		if ( f1->addMsg( f2, m->mid(), src() ) )
			return m->mid();
		else
			delete m; // Nasty, but rare.
	}
	return Msg::Null;
}

////////////////////////////////////////////////////////////////
// DestFuncs
////////////////////////////////////////////////////////////////

void Shell::process( const ProcInfo* p, const Eref& e )
{
	quit_ = 1;
}


void Shell::setName( string name )
{
	name_ = name;
}

string Shell::getName() const
{
	return name_;
}

void Shell::setQuit( bool val )
{
	quit_ = val;
}

bool Shell::getQuit() const
{
	return quit_;
}

void Shell::handleGet( Eref e, const Qinfo* q, const char* arg )
{
	getBuf_.resize( q->size() );
	memcpy( &getBuf_[0], arg, q->size() );
	// Instead of deleting and recreating the msg, it could be a 
	// permanent msg on this object, reaching out whenever needed
	// to targets.
}

const char* Shell::getBuf() const
{
	if ( getBuf_.size() > 0 )
		return &( getBuf_[0] );
	return 0;
}


/**
 * This function handles the message request to create an Element.
 * This request specifies the Id of the new Element and is handled on
 * all nodes.
 *
 * In due course we also have to set up the node decomposition of the
 * Element, but for now the num indicates the total # of array entries.
 * This gets a bit complicated if the Element is a multidim array.
 */
void Shell::create( string type, Id parent, Id newElm, string name )
{
	innerCreate( type, parent, newElm, name );
}

/**
 * This function actually creates the object.
 */
void Shell::innerCreate( string type, Id parent, Id newElm, string name )
{
	const Cinfo* c = Cinfo::find( type );
	bool ret = 0;
	unsigned int num = 1; // hack till I figure out how to set up allocs.
	if ( c ) {
		Element* pa = parent();
		if ( !pa ) {
			stringstream ss;
			ss << "create: Parent Element'" << parent << "' not found. No Element created";
		}
		ret = c->create( newElm, name, num );
	} else {
		stringstream ss;
		ss << "create: Class '" << type << "' not known. No Element created";
		warning( ss.str() );
	}
	// Send back ack with status
	// ack.send( e, ret );
}

void Shell::destroy( Id eid)
{
	eid.destroy();
}


// I really also want to put in a message type. But each message has its
// own features and these may well be done separately
void Shell::addmsg( Id src, Id dest, string srcfield, string destfield )
{
}

void Shell::warning( const string& text )
{
	cout << "Warning: Shell:: " << text << endl;
}

void Shell::error( const string& text )
{
	cout << "Error: Shell:: " << text << endl;
}

void Shell::handleAckCreate()
{
	;
}

void Shell::handleAckDelete()
{
	;
}

////////////////////////////////////////////////////////////////////////
// Some static utility functions
////////////////////////////////////////////////////////////////////////

// Statid func for returning the pet ProcInfo of the shell.
const ProcInfo* Shell::procInfo()
{
	return &p_;
}

/**
 * Static global, returns contents of shell buffer.
 */
const char* Shell::buf() 
{
	static Id shellid;
	static Element* shell = shellid();
	assert( shell );
	return (reinterpret_cast< Shell* >(shell->data( 0 )) )->getBuf();
}

// Function to assign hardware availability
void Shell::setHardware( 
	bool isSingleThreaded, unsigned int numCores, unsigned int numNodes )
{
	isSingleThreaded_ = isSingleThreaded;
	Qinfo::addSimGroup( 1 ); // This is the parser thread.
	if ( !isSingleThreaded ) {
		// Create the parser and the gui threads.
		numCores_ = numCores;
		numNodes_ = numNodes;
		// The zero queue is for system calls. Then there is one queue
		// per local thread. Each off-node gets another queue.
		// Note the more complex 'group' orgn for
		// eventual highly multithreaded architectures, discussed in
		// NOTES 10 Dec 2009.
		// Qinfo::setNumQs( numCores_ + numNodes_, 1024 );
		//
	} else {
		numCores_ = 1;
		numNodes_ = 1;
		// Qinfo::setNumQs( 1, 1024 );
	}
}

/**
 * Regular shell function that requires that the information about the
 * hardware have been loaded in. For now the function just assigns SimGroups
 */
void Shell::loadBalance()
{
	// Need more info here on how to set up groups distributed over
	// nodes. In fact this will have to be computed _after_ the
	// simulation is loaded. Will also need quite a bit of juggling between
	// nodes when things get really scaled up.
	//
	// Note that the messages have to be rebuilt after this call.
	if ( !isSingleThreaded_ ) {
		for ( unsigned int i = 0; i < numNodes_; ++i )
			Qinfo::addSimGroup( numCores_ ); //These are the worker threads.
	}
}

unsigned int Shell::numCores()
{
	return numCores_;
}

////////////////////////////////////////////////////////////////////////
// Functions for setting off clocked processes.

void Shell::start( double runtime )
{
	Id clockId( 1 );
	Element* clocke = clockId();
	Qinfo q;
	if ( isSingleThreaded_ ) {
		// SetGet< double >::set( clocke, runTime );
		// clock->start( clocke, &q, runTime );
		Clock *clock = reinterpret_cast< Clock* >( clocke->data( 0 ) );
		clock->start( clockId.eref(), &q, runtime );
		return;
	}

	vector< ThreadInfo > ti( numCores_ );
	pthread_mutex_t sortMutex;
	pthread_mutex_init( &sortMutex, NULL );
	pthread_mutex_t timeMutex;
	pthread_mutex_init( &timeMutex, NULL );

	/*
	for ( unsigned int i = 0; i < numCores_; ++i ) {
		ti[i].clocke = clocke;
		ti[i].qinfo = &q;
		ti[i].runtime = runtime;
		ti[i].threadId = i;
		ti[i].sortMutex = &sortMutex;
		ti[i].timeMutex = &timeMutex;
	}
	*/

	unsigned int j = 0;
	for ( unsigned int i = 1; i < Qinfo::numSimGroup(); ++i ) {
		for ( unsigned short k = 0; k < Qinfo::simGroup( i )->numThreads; ++k ) {
			ti[j].clocke = clocke;
			ti[j].qinfo = &q;
			ti[j].runtime = runtime;
			ti[j].threadId = j;
			ti[j].threadIndexInGroup = j - Qinfo::simGroup( i )->startThread + 1;
			ti[j].groupId = i;
			ti[j].outQid = Qinfo::simGroup(i)->startThread + k;
			ti[j].sortMutex = &sortMutex;
			ti[j].timeMutex = &timeMutex;
			j++;
		}
	}

	assert( j == numCores_ );

	pthread_t* threads = new pthread_t[ numCores_ ];
	pthread_attr_t attr;

	pthread_attr_init( &attr );
	pthread_attr_setdetachstate( &attr, PTHREAD_CREATE_JOINABLE );
	pthread_barrier_t barrier;
	if ( pthread_barrier_init( &barrier, NULL, numCores_ ) ) {
		cout << "Error: Shell::start: Unable to init barrier\n";
		exit( -1 );
	}
	Clock* clock = reinterpret_cast< Clock* >( clocke->data( 0 ) );
	clock->setBarrier( &barrier );
	clock->setNumPendingThreads( 0 ); // Used for clock scheduling
	clock->setNumThreads( numCores_ ); // Used for clock scheduling
	// pthread_t threads[ numCores_ ];
	for ( unsigned int i = 0; i < numCores_; ++i ) {
		int ret = pthread_create( 
			&threads[i], NULL, Clock::threadStartFunc, 
			reinterpret_cast< void* >( &ti[i] )
		);
		if ( ret ) {
			cout << "Error: Shell::start: Unable to create threads\n";
			exit( -1 );
		}
	}

	// Clean up.
	for ( unsigned int i = 0; i < numCores_; ++i ) {
		void* status;
		int ret = pthread_join( threads[ i ], &status );
		if ( ret ) {
			cout << "Error: Shell::start: Unable to join threads\n";
			exit( -1 );
		}
	}
		// cout << "Shell::start: Threads joined successfully\n";
		// cout << "Completed time " << runtime << " on " << numCores_ << " threads\n";

	delete[] threads;
	pthread_attr_destroy( &attr );
	pthread_barrier_destroy( &barrier );
	pthread_mutex_destroy( &sortMutex );
	pthread_mutex_destroy( &timeMutex );
}

void Shell::setclock( unsigned int tickNum, double dt, unsigned int stage )
{
	Eref ce = Id( 1 ).eref();
	SetGet3< unsigned int, double, unsigned int >::set( ce, "setupTick",
		tickNum, dt, stage );
}

////////////////////////////////////////////////////////////////////////

// Deprecated. Will go into Shell.
bool set( Eref& dest, const string& destField, const string& val )
{
	static Id shellid;
	static BindIndex setBinding = 0; // Need to fix up.
	Element* shell = shellid();
	SrcFinfo1< string > sf( "set", "dummy", 0 );

	FuncId fid = dest.element()->cinfo()->getOpFuncId( destField );
	const OpFunc* func = dest.element()->cinfo()->getOpFunc( fid );
	if ( func ) {
		if ( func->checkFinfo( &sf ) ) {
			shell->clearBinding( setBinding );
			shell->clearBinding( setBinding );
			Eref shelle = shellid.eref();
			Msg* m = new SingleMsg( shelle, dest );
			shell->addMsgAndFunc( m->mid(), fid, setBinding );
			sf.send( shelle, Shell::procInfo(), val );
			return 1;
		} else {
			cout << "set::Type mismatch" << dest << "." << destField << endl;
		}
	} else {
		cout << "set::Failed to find " << dest << "." << destField << endl;
	}
	return 0;
}

bool get( const Eref& dest, const string& destField )
{
	static Id shellid;
	static BindIndex getBindIndex = 0;

	static const Finfo* reqFinfo = shellCinfo->findFinfo( "requestGet" );
	static const SrcFinfo1< FuncId >* rf = 
		dynamic_cast< const SrcFinfo1< FuncId >* >( reqFinfo );
	static FuncId retFunc = shellCinfo->getOpFuncId( "handleGet" );
	static SrcFinfo1< string > sf( "get", "dummy", 0 );

	static Element* shell = shellid();
	static Eref shelle( shell, 0 );

	FuncId fid = dest.element()->cinfo()->getOpFuncId( destField );
	const OpFunc* func = dest.element()->cinfo()->getOpFunc( fid );

	assert( rf != 0 );

	if ( func ) {
		if ( func->checkFinfo( &sf ) ) {
			shell->clearBinding( getBindIndex );
			Msg* m = new SingleMsg( shelle, dest );
			shell->addMsgAndFunc( m->mid(), fid, getBindIndex );
			rf->send( shelle, Shell::procInfo(), retFunc );
			// Now, dest has to clearQ, do its stuff, then src has to clearQ
			return 1;
		} else {
			cout << "set::Type mismatch" << dest << "." << destField << endl;
		}
	} else {
		cout << "set::Failed to find " << dest << "." << destField << endl;
	}
	return 0;
}

