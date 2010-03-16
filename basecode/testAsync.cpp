/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#include "header.h"
#include "Neutral.h"
#include "Dinfo.h"
#include "Message.h"
#include <queue>
#include "../biophysics/Synapse.h"
#include "../biophysics/IntFire.h"
#include "SparseMatrix.h"
#include "SparseMsg.h"
#include "PsparseMsg.h"
#include "../randnum/randnum.h"
#include "../scheduling/Tick.h"
#include "../scheduling/TickPtr.h"
#include "../scheduling/Clock.h"

void showFields()
{
	const Cinfo* nc = Neutral::initCinfo();
	Id i1 = Id::nextId();
	bool ret = nc->create( i1, "test1", 1 );
	assert( ret );

	i1.eref().element()->showFields();
	cout << "." << flush;

	delete i1();
}

void insertIntoQ( )
{
	const Cinfo* nc = Neutral::initCinfo();
	unsigned int size = 100;

	const DestFinfo* df = dynamic_cast< const DestFinfo* >(
		nc->findFinfo( "set_name" ) );
	assert( df != 0 );
	FuncId fid = df->getFid();

	Id i1 = Id::nextId();
	Id i2 = Id::nextId();
	bool ret = nc->create( i1, "test1", size );
	assert( ret );
	ret = nc->create( i2, "test2", size );
	assert( ret );

	Eref e1 = i1.eref();
	Eref e2 = i2.eref();

	Msg* m = new SingleMsg( e1, e2 );
	

	for ( unsigned int i = 0; i < size; ++i ) {
		char temp[20];
		sprintf( temp, "objname_%d", i );
		string stemp( temp );
		char buf[200];

		// This simulates a sendTo
		Conv< string > conv( stemp );
		unsigned int size = conv.val2buf( buf );
		// unsigned int size = Conv< string >::val2buf( buf, stemp );
		Qinfo qi( 1, i, size + sizeof( DataId ), 1, 1 );

		*reinterpret_cast< DataId* >( buf + size ) = DataId( i );

		MsgFuncBinding b( m->mid(), fid );

		// addToQ( qid, Binding, argbuf )
		qi.addToQ( 0, b, buf );
	}
	ProcInfo p;
	Qinfo::clearQ( &p );

	for ( unsigned int i = 0; i < size; ++i ) {
		char temp[20];
		sprintf( temp, "objname_%d", i );
		string name = ( reinterpret_cast< Neutral* >(e2.element()->data( i )) )->getName();
		assert( name == temp );
	}
	cout << "." << flush;

	delete m;
	delete i1();
	delete i2();
}

void testSendMsg()
{
	const Cinfo* nc = Neutral::initCinfo();
	unsigned int size = 100;

	const DestFinfo* df = dynamic_cast< const DestFinfo* >(
		nc->findFinfo( "set_name" ) );
	assert( df != 0 );
	FuncId fid = df->getFid();

	Id i1 = Id::nextId();
	Id i2 = Id::nextId();
	bool ret = nc->create( i1, "test1", size );
	assert( ret );
	ret = nc->create( i2, "test2", size );
	assert( ret );

	Eref e1 = i1.eref();
	Eref e2 = i2.eref();

	Msg* m = new OneToOneMsg( e1.element(), e2.element() );

	
	ProcInfo p;
	
	// Defaults to BindIndex of 0.
	SrcFinfo1<string> s( "test", "" );
	e1.element()->addMsgAndFunc( m->mid(), fid, s.getBindIndex() );

	for ( unsigned int i = 0; i < size; ++i ) {
		char temp[20];
		sprintf( temp, "send_to_e2_%d", i );
		string stemp( temp );
		s.send( Eref( e1.element(), i ), &p, stemp );
	}
	Qinfo::clearQ( &p );

	for ( unsigned int i = 0; i < size; ++i ) {
		char temp[20];
		sprintf( temp, "send_to_e2_%d", i );
		assert( reinterpret_cast< Neutral* >(e2.element()->data( i ))->getName()
			== temp );
	}
	cout << "." << flush;

	delete i1();
	delete i2();
}

void testCreateMsg()
{
	const Cinfo* nc = Neutral::initCinfo();
	unsigned int size = 100;
	Id i1 = Id::nextId();
	Id i2 = Id::nextId();
	bool ret = nc->create( i1, "test1", size );
	assert( ret );
	ret = nc->create( i2, "test2", size );
	assert( ret );

	Eref e1 = i1.eref();
	Eref e2 = i2.eref();
	ProcInfo p;

	ret = add( e1.element(), "child", e2.element(), "parent" );
	
	assert( ret );

	const Finfo* f = nc->findFinfo( "child" );

	for ( unsigned int i = 0; i < size; ++i ) {
		const SrcFinfo0* sf = dynamic_cast< const SrcFinfo0* >( f );
		assert( sf != 0 );
		sf->send( Eref( e1.element(), i ), &p );
	}
	Qinfo::clearQ( &p );

	/*
	for ( unsigned int i = 0; i < size; ++i )
		cout << i << "	" << reinterpret_cast< Neutral* >(e2.element()->data( i ))->getName() << endl;

*/
	cout << "." << flush;
	delete i1();
	delete i2();
}

void testSet()
{
	const Cinfo* nc = Neutral::initCinfo();
	unsigned int size = 100;
	string arg;
	Id i2 = Id::nextId();
	bool ret = nc->create( i2, "test2", size );
	assert( ret );
	ProcInfo p;

	Eref e2 = i2.eref();
	
	for ( unsigned int i = 0; i < size; ++i ) {
		char temp[20];
		sprintf( temp, "set_e2_%d", i );
		string stemp( temp );
		Eref dest( e2.element(), i );
		set( dest, "set_name", stemp );
		Qinfo::clearQ( &p );
	}

	for ( unsigned int i = 0; i < size; ++i ) {
		char temp[20];
		sprintf( temp, "set_e2_%d", i );
		assert( reinterpret_cast< Neutral* >(e2.element()->data( i ))->getName()
			== temp );
	}

	cout << "." << flush;

	delete i2();
}

void testGet()
{
	const Cinfo* nc = Neutral::initCinfo();
	unsigned int size = 100;
	string arg;
	Id i2 = Id::nextId();
	bool ret = nc->create( i2, "test2", size );
	assert( ret );
	Element* shell = Id()();
	ProcInfo p;

	Eref e2 = i2.eref();
	
	for ( unsigned int i = 0; i < size; ++i ) {
		char temp[20];
		sprintf( temp, "get_e2_%d", i );
		string stemp( temp );
		reinterpret_cast< Neutral* >(e2.element()->data( i ))->setName( temp );
	}

	for ( unsigned int i = 0; i < size; ++i ) {
		string stemp;
		Eref dest( e2.element(), i );

			// I don't really want an array of SetGet/Shells to originate
			// get requests, but just
			// to test that it works from anywhere...
		if ( get( dest, "get_name" ) ) {
			Qinfo::clearQ( &p ); // Request goes to e2
			// shell->clearQ(); // Response comes back to e1
			Qinfo::clearQ( &p ); // Response comes back to e1

			stemp = ( reinterpret_cast< Shell* >(shell->data( 0 )) )->getBuf();
			// cout << i << "	" << stemp << endl;
			char temp[20];
			sprintf( temp, "get_e2_%d", i );
			assert( stemp == temp );
		}
	}

	cout << "." << flush;
	delete i2();
}

void testSetGet()
{
	const Cinfo* nc = Neutral::initCinfo();
	unsigned int size = 100;
	string arg;
	Id i2 = Id::nextId();
	bool ret = nc->create( i2, "test2", size );
	assert( ret );

	
	for ( unsigned int i = 0; i < size; ++i ) {
		Eref e2( i2(), i );
		char temp[20];
		sprintf( temp, "sg_e2_%d", i );
		bool ret = Field< string >::set( e2, "name", temp );
		assert( ret );
		assert( reinterpret_cast< Neutral* >(e2.data())->getName() == temp );
	}

	for ( unsigned int i = 0; i < size; ++i ) {
		Eref e2( i2(), i );
		char temp[20];
		sprintf( temp, "sg_e2_%d", i );
		string ret = Field< string >::get( e2, "name" );
		assert( ret == temp );
	}

	cout << "." << flush;
	delete i2();
}

void testSetGetDouble()
{
	static const double EPSILON = 1e-9;
	const Cinfo* ic = IntFire::initCinfo();
	unsigned int size = 100;
	string arg;
	Id i2 = Id::nextId();
	bool ret = ic->create( i2, "test2", size );
	assert( ret );

	// i2()->showFields();

	
	for ( unsigned int i = 0; i < size; ++i ) {
		Eref e2( i2(), i );
		double temp = i;
		bool ret = Field< double >::set( e2, "Vm", temp );
		assert( ret );
		assert( 
			fabs ( reinterpret_cast< IntFire* >(e2.data())->getVm() - temp ) <
				EPSILON ); 
	}

	for ( unsigned int i = 0; i < size; ++i ) {
		Eref e2( i2(), i );
		double temp = i;
		double ret = Field< double >::get( e2, "Vm" );
		assert( fabs ( temp - ret ) < EPSILON );
	}

	cout << "." << flush;
	delete i2();
}

void testSetGetSynapse()
{
	static const double EPSILON = 1e-9;
	const Cinfo* ic = IntFire::initCinfo();
	const Cinfo* sc = Synapse::initCinfo();
	unsigned int size = 100;
	string arg;
	Id i2 = Id::nextId();
	bool ret = ic->create( i2, "test2", size );
	assert( ret );

	// SynElement syn( sc, i2() );
	FieldElement< Synapse, IntFire, &IntFire::synapse > syn( sc, i2(), &IntFire::getNumSynapses, &IntFire::setNumSynapses );

	assert( syn.numData() == 0 );
	for ( unsigned int i = 0; i < size; ++i ) {
		Eref e2( i2(), i );
		ret = Field< unsigned int >::set( e2, "numSynapses", i );
		assert( ret );
	}
	assert( syn.numData() == ( size * (size - 1) ) / 2 );
	// cout << "NumSyn = " << syn.numData() << endl;
	
	for ( unsigned int i = 0; i < size; ++i ) {
		for ( unsigned int j = 0; j < i; ++j ) {
			DataId di( i, j );
			Eref syne( &syn, di );
			double temp = i * 1000 + j ;
			bool ret = Field< double >::set( syne, "delay", temp );
			assert( ret );
			assert( 
			fabs ( reinterpret_cast< Synapse* >(syne.data())->getDelay() - temp ) <
				EPSILON ); 
		}
	}
	cout << "." << flush;
	delete i2();
}

void testSetGetVec()
{
	static const double EPSILON = 1e-9;
	const Cinfo* ic = IntFire::initCinfo();
	const Cinfo* sc = Synapse::initCinfo();
	unsigned int size = 100;
	string arg;
	Id i2 = Id::nextId();
	bool ret = ic->create( i2, "test2", size );
	assert( ret );
//	SynElement syn( sc, i2() );
	FieldElement< Synapse, IntFire, &IntFire::synapse > syn( sc, i2(), &IntFire::getNumSynapses, &IntFire::setNumSynapses );

	assert( syn.numData() == 0 );
	vector< unsigned int > numSyn( size, 0 );
	for ( unsigned int i = 0; i < size; ++i )
		numSyn[i] = i;
	
	Eref e2( i2(), 0 );
	// Here we test setting a 1-D vector
	ret = Field< unsigned int >::setVec( e2, "numSynapses", numSyn );
	assert( ret );
	unsigned int nd = syn.numData();
	assert( nd == ( size * (size - 1) ) / 2 );
	// cout << "NumSyn = " << nd << endl;
	
	// Here we test setting a 2-D array with different dims on each axis.
	vector< double > delay( nd, 0.0 );
	unsigned int k = 0;
	for ( unsigned int i = 0; i < size; ++i ) {
		for ( unsigned int j = 0; j < i; ++j ) {
			delay[k++] = i * 1000 + j;
		}
	}

	Eref se( &syn, 0 );
	ret = Field< double >::setVec( se, "delay", delay );
	for ( unsigned int i = 0; i < size; ++i ) {
		for ( unsigned int j = 0; j < i; ++j ) {
			DataId di( i, j );
			Eref syne( &syn, di );
			double temp = i * 1000 + j ;
			assert( 
			fabs ( reinterpret_cast< Synapse* >(syne.data())->getDelay() - temp ) <
				EPSILON ); 
		}
	}
	cout << "." << flush;
	delete i2();
}

void testSendSpike()
{
	static const double EPSILON = 1e-9;
	static const double WEIGHT = -1.0;
	static const double TAU = 1.0;
	static const double DT = 0.1;
	const Cinfo* ic = IntFire::initCinfo();
	const Cinfo* sc = Synapse::initCinfo();
	unsigned int size = 100;
	string arg;
	Id i2 = Id::nextId();
	bool ret = ic->create( i2, "test2", size );
	assert( ret );
	Eref e2 = i2.eref();
	//SynElement syn( sc, i2() );
	FieldElement< Synapse, IntFire, &IntFire::synapse > syn( sc, i2(), &IntFire::getNumSynapses, &IntFire::setNumSynapses );

	assert( syn.numData() == 0 );
	for ( unsigned int i = 0; i < size; ++i ) {
		Eref er( i2(), i );
		bool ret = Field< unsigned int >::set( er, "numSynapses", i );
		assert( ret );
	}
	assert( syn.numData() == ( size * (size - 1) ) / 2 );

	DataId di( 1, 0 ); // DataId( data, field )
	Eref syne( &syn, di );
	reinterpret_cast< Synapse* >(syne.data())->setWeight( WEIGHT );

	ret = SingleMsg::add( e2, "spike", syne, "addSpike" );
	assert( ret );

	reinterpret_cast< IntFire* >(e2.data())->setVm( 1.0 );
	// ret = SetGet1< double >::set( e2, "Vm", 1.0 );
	ProcInfo p;
	p.dt = DT;
	reinterpret_cast< IntFire* >(e2.data())->process( &p, e2 );
	// At this stage we have sent the spike, so e2.data::Vm should be -1e-7.
	double Vm = reinterpret_cast< IntFire* >(e2.data())->getVm();
	assert( fabs( Vm + 1e-7) < EPSILON );
	// Test that the spike message is in the queue.
	assert( Qinfo::outQ_[0].size() == sizeof( Qinfo ) + sizeof( double ) );

	Qinfo::clearQ( &p );
	assert( Qinfo::outQ_[0].size() == 0 );

	/*
	// Warning: the 'get' function calls clearQ also.
	Vm = SetGet1< double >::get( e2, "Vm" );
	assert( fabs( Vm + 1e-7) < EPSILON );
	*/

	Eref synParent( e2.element(), 1 );
	reinterpret_cast< IntFire* >(synParent.data())->setTau( TAU );

	reinterpret_cast< IntFire* >(synParent.data())->process( &p, synParent);
	Vm = Field< double >::get( synParent, "Vm" );
	assert( fabs( Vm - WEIGHT * ( 1.0 - DT / TAU ) ) < EPSILON );
	// cout << "Vm = " << Vm << endl;
	cout << "." << flush;
	delete i2();
}

void printSparseMatrix( const SparseMatrix< unsigned int >& m)
{
	unsigned int nRows = m.nRows();
	unsigned int nCols = m.nColumns();
	
	for ( unsigned int i = 0; i < nRows; ++i ) {
		cout << "[	";
		for ( unsigned int j = 0; j < nCols; ++j ) {
			cout << m.get( i, j ) << "	";
		}
		cout << "]\n";
	}
	const unsigned int *n;
	const unsigned int *c;
	for ( unsigned int i = 0; i < nRows; ++i ) {
		unsigned int num = m.getRow( i, &n, &c );
		for ( unsigned int j = 0; j < num; ++j )
			cout << n[j] << "	";
	}
	cout << endl;

	for ( unsigned int i = 0; i < nRows; ++i ) {
		unsigned int num = m.getRow( i, &n, &c );
		for ( unsigned int j = 0; j < num; ++j )
			cout << c[j] << "	";
	}
	cout << endl;
	cout << endl;
}

void testSparseMatrix()
{
	static unsigned int preN[] = { 1, 2, 3, 4, 5, 6, 7 };
	static unsigned int postN[] = { 1, 3, 4, 5, 6, 2, 7 };
	static unsigned int preColIndex[] = { 0, 4, 0, 1, 2, 3, 4 };
	static unsigned int postColIndex[] = { 0, 1, 1, 1, 2, 0, 2 };

	SparseMatrix< unsigned int > m( 3, 5 );
	unsigned int nRows = m.nRows();
	unsigned int nCols = m.nColumns();

	m.set( 0, 0, 1 );
	m.set( 0, 4, 2 );
	m.set( 1, 0, 3 );
	m.set( 1, 1, 4 );
	m.set( 1, 2, 5 );
	m.set( 2, 3, 6 );
	m.set( 2, 4, 7 );

	const unsigned int *n;
	const unsigned int *c;
	unsigned int k = 0;
	for ( unsigned int i = 0; i < nRows; ++i ) {
		unsigned int num = m.getRow( i, &n, &c );
		for ( unsigned int j = 0; j < num; ++j ) {
			assert( n[j] == preN[ k ] );
			assert( c[j] == preColIndex[ k ] );
			k++;
		}
	}
	assert( k == 7 );

	// printSparseMatrix( m );

	m.transpose();

	k = 0;
	for ( unsigned int i = 0; i < nCols; ++i ) {
		unsigned int num = m.getRow( i, &n, &c );
		for ( unsigned int j = 0; j < num; ++j ) {
			assert( n[j] == postN[ k ] );
			assert( c[j] == postColIndex[ k ] );
			k++;
		}
	}
	assert( k == 7 );

	cout << "." << flush;
}

void testSparseMatrixBalance()
{
	SparseMatrix< unsigned int > m( 3, 6 );
	unsigned int nRows = m.nRows();
	unsigned int nCols = m.nColumns();

	for ( unsigned int i = 0; i < nRows; ++i ) {
		for ( unsigned int j = 0; j < nCols; ++j ) {
			m.set( i, j, 100 * i + j );
		}
	}

	// printSparseMatrix( m );
	sparseMatrixBalance( 2, m );
	// printSparseMatrix( m );
	
	for ( unsigned int i = 0; i < nRows; ++i ) {
		unsigned int threadNum = i % 2;
		for ( unsigned int j = 0; j < nCols; ++j ) {
			if ( ( 2 * j ) / nCols == threadNum )
				assert( m.get( i, j ) ==  100 * ( i / 2 ) + j );
			else
				assert( m.get( i, j ) ==  0 );
		}
	}

	cout << "." << flush;
}

void printGrid( Element* e, const string& field, double min, double max )
{
	static string icon = " .oO@";
	unsigned int yside = sqrt( double ( e->numData() ) );
	unsigned int xside = e->numData() / yside;
	if ( e->numData() % yside > 0 )
		xside++;
	
	for ( unsigned int i = 0; i < e->numData(); ++i ) {
		if ( ( i % xside ) == 0 )
			cout << endl;
		Eref er( e, i );
		double Vm = Field< double >::get( er, field );
		int shape = 5.0 * ( Vm - min ) / ( max - min );
		if ( shape > 4 )
			shape = 4;
		if ( shape < 0 )
			shape = 0;
		cout << icon[ shape ];
	}
	cout << endl;
}


void testSparseMsg()
{
	static const unsigned int qSize[] =
		{ 838, 10, 6, 18, 36, 84, 150, 196, 258, 302 };
	static const unsigned int NUMSYN = 104576;
	static const double thresh = 0.2;
	static const double Vmax = 1.0;
	static const double refractoryPeriod = 0.4;
	static const double weightMax = 0.02;
	static const double delayMax = 4;
	static const double timestep = 0.2;
	static const double connectionProbability = 0.1;
	static const unsigned int runsteps = 5;
	const Cinfo* ic = IntFire::initCinfo();
	const Cinfo* sc = Synapse::initCinfo();
	unsigned int size = 1024;
	string arg;

	mtseed( 5489UL ); // The default value, but better to be explicit.

	Id i2 = Id::nextId();
	bool ret = ic->create( i2, "test2", size );
	assert( ret );
	Eref e2 = i2.eref();
	FieldElement< Synapse, IntFire, &IntFire::synapse > syn( sc, i2(), &IntFire::getNumSynapses, &IntFire::setNumSynapses );

	assert( syn.numData() == 0 );

	DataId di( 1, 0 ); // DataId( data, field )
	Eref syne( &syn, di );

	ret = SparseMsg::add( e2.element(), "spike", &syn, "addSpike", 
		connectionProbability );
	assert( ret );

	unsigned int nd = syn.numData();
//	cout << "Num Syn = " << nd << endl;
	assert( nd == NUMSYN );
	vector< double > temp( size, 0.0 );
	for ( unsigned int i = 0; i < size; ++i )
		temp[i] = mtrand() * Vmax;

	ret = Field< double >::setVec( e2, "Vm", temp );
	assert( ret );
	/*
	for ( unsigned int i = 0; i < 40; ++i )
		cout << reinterpret_cast< IntFire* >( e2.element()->data( i ) )->getVm() << "	" << temp[i] << endl;
		*/
	temp.clear();
	temp.resize( size, thresh );
	ret = Field< double >::setVec( e2, "thresh", temp );
	assert( ret );
	temp.clear();
	temp.resize( size, refractoryPeriod );
	ret = Field< double >::setVec( e2, "refractoryPeriod", temp );
	assert( ret );

	vector< double > weight;
	weight.reserve( nd );
	vector< double > delay;
	delay.reserve( nd );
	for ( unsigned int i = 0; i < size; ++i ) {
		unsigned int numSyn = syne.element()->numData2( i );
		for ( unsigned int j = 0; j < numSyn; ++j ) {
			weight.push_back( mtrand() * weightMax );
			delay.push_back( mtrand() * delayMax );
		}
	}
	ret = Field< double >::setVec( syne, "weight", weight );
	assert( ret );
	ret = Field< double >::setVec( syne, "delay", delay );
	assert( ret );

	// printGrid( i2(), "Vm", 0, thresh );

	ProcInfo p;
	p.dt = timestep;
	/*
	IntFire* ifire100 = reinterpret_cast< IntFire* >( e2.element()->data( 100 ) );
	IntFire* ifire900 = reinterpret_cast< IntFire* >( e2.element()->data( 900 ) );
	*/

	for ( unsigned int i = 0; i < runsteps; ++i ) {
		p.currTime += p.dt;
		i2()->process( &p );
		unsigned int numWorkerThreads = 1;
		unsigned int startThread = 1;
		if ( Qinfo::numSimGroup() >= 2 ) {
			numWorkerThreads = Qinfo::simGroup( 1 )->numThreads;
			startThread = Qinfo::simGroup( 1 )->startThread;
		}
		unsigned int totOutqEntries = 0;
		for ( unsigned int j = 0; j < numWorkerThreads; ++j )
			totOutqEntries += Qinfo::outQ_[ j ].size();
		assert( totOutqEntries / ( sizeof( Qinfo ) + sizeof( double ) ) == qSize[i] );
		// cout << p.currTime << "	" << ifire100->getVm() << "	" << ifire900->getVm() << endl;
		// cout << "T = " << p.currTime << ", Q size = " << Qinfo::q_[0].size() << endl;
		Qinfo::clearQ( &p );
//		i2()->process( &p );
		// printGrid( i2(), "Vm", 0, thresh );
		// sleep(1);
	}
	// printGrid( i2(), "Vm", 0, thresh );

	cout << "." << flush;
	delete i2();
}

void testUpValue()
{
	static const double EPSILON = 1e-9;
	const Cinfo* cc = Clock::initCinfo();
	const Cinfo* tc = Tick::initCinfo();
	unsigned int size = 10;
	Id clock = Id::nextId();
	bool ret = cc->create( clock, "clock", 1 );
	assert( ret );

	Eref clocker = clock.eref();
	//SynElement syn( sc, i2() );
	FieldElement< Tick, Clock, &Clock::getTick > ticke( tc, clock(), &Clock::getNumTicks, &Clock::setNumTicks );

	assert( ticke.numData() == 0 );
	ret = Field< unsigned int >::set( clocker, "numTicks", size );
	assert( ret );
	assert( ticke.numData() == size );


	for ( unsigned int i = 0; i < size; ++i ) {
		DataId di( 0, i ); // DataId( data, field )
		Eref te( &ticke, di );
		double dt = i;
		ret = Field< double >::set( te, "dt", dt );
		assert( ret );
		double val = Field< double >::get( te, "localdt" );
		assert( fabs( dt - val ) < EPSILON );

		dt *= 10.0;
		ret = Field< double >::set( te, "localdt", dt );
		assert( ret );
		val = Field< double >::get( te, "dt" );
		assert( fabs( dt - val ) < EPSILON );
	}
	cout << "." << flush;
	delete clock();
}

/**
 * This sets up a reciprocal shared Msg in which the incoming value gets
 * appended onto the corresponding value of the target. Also, as soon
 * as any of the s1 or s2 are received, the target sends out an s0 call.
 * All this is tallied for validating the unit test.
 */

static SrcFinfo0 s0( "s0", "");
class Test: public Data
{
	public:
		Test()
			: numAcks_( 0 )
		{;}

		void process( const ProcInfo* p, const Eref& e )
		{;}

		void handleS0() {
			numAcks_++;
		}

		void handleS1( Eref e, const Qinfo* q, string s ) {
			ProcInfo p;
			s_ = s + s_;
			s0.send( e, &p, 0 );
		}

		void handleS2( Eref e, const Qinfo* q, int i1, int i2 ) {
			ProcInfo p;
			i1_ += 10 * i1;
			i2_ += 10 * i2;
			s0.send( e, &p, 0 );
		}

		static Finfo* sharedVec[ 6 ];

		static const Cinfo* initCinfo()
		{
			static Finfo * testFinfos[] = {
				new SharedFinfo( "shared", "",
					sharedVec, sizeof( sharedVec ) / sizeof( const Finfo * ) ),
			};

			static Cinfo testCinfo(
				"Test",
				0,
				testFinfos,
				sizeof( testFinfos ) / sizeof( Finfo* ),
				new Dinfo< Test >()
			);
	
			return &testCinfo;
		}

		string s_;
		int i1_;
		int i2_;
		int numAcks_;
};

Finfo* Test::sharedVec[6];

void testSharedMsg()
{
	static SrcFinfo1< string > s1( "s1", "" );
	static SrcFinfo2< int, int > s2( "s2", "" );
	static DestFinfo d0( "d0", "",
		new OpFunc0< Test >( & Test::handleS0 ) );
	static DestFinfo d1( "d1", "", 
		new EpFunc1< Test, string >( &Test::handleS1 ) );
	static DestFinfo d2( "d2", "", 
		new EpFunc2< Test, int, int >( &Test::handleS2 ) );

	Test::sharedVec[0] = &s0;
	Test::sharedVec[1] = &d0;
	Test::sharedVec[2] = &s1;
	Test::sharedVec[3] = &d1;
	Test::sharedVec[4] = &s2;
	Test::sharedVec[5] = &d2;
	
	Id t1 = Id::nextId();
	Id t2 = Id::nextId();
	bool ret = Test::initCinfo()->create( t1, "test1", 1 );
	assert( ret );
	ret = Test::initCinfo()->create( t2, "test2", 1 );
	assert( ret );

	// Assign initial values
	Test* tdata1 = reinterpret_cast< Test* >( t1.eref().data() );
	tdata1->s_ = "tdata1";
	tdata1->i1_ = 1;
	tdata1->i2_ = 2;

	Test* tdata2 = reinterpret_cast< Test* >( t2.eref().data() );
	tdata2->s_ = "TDATA2";
	tdata2->i1_ = 5;
	tdata2->i2_ = 6;

	// Set up message. The actual routine is in Shell.cpp, but here we
	// do it independently.
	
	const Finfo* shareFinfo = Test::initCinfo()->findFinfo( "shared" );
	assert( shareFinfo != 0 );
	Msg* m = new OneToOneMsg( t1(), t2() );
	assert( m != 0 );
	ret = shareFinfo->addMsg( shareFinfo, m->mid(), t1() );
	assert( ret );

	// Display stuff. Need to figure out how to unit test this.
	// t1()->showMsg();
	// t2()->showMsg();

	// Send messages
	ProcInfo p;
	string arg1 = " hello ";
	s1.send( t1.eref(), &p, arg1 );
	s2.send( t1.eref(), &p, 100, 200 );

	Qinfo::clearQ( &p );
	Qinfo::clearQ( &p );

	string arg2 = " goodbye ";
	s1.send( t2.eref(), &p, arg2, 0 );
	s2.send( t2.eref(), &p, 500, 600, 0 );

	Qinfo::clearQ( &p );
	Qinfo::clearQ( &p );

	/*
	cout << "data1: s=" << tdata1->s_ << 
		", i1=" << tdata1->i1_ << ", i2=" << tdata1->i2_ << 
		", numAcks=" << tdata1->numAcks_ << endl;
	cout << "data2: s=" << tdata2->s_ << 
		", i1=" << tdata2->i1_ << ", i2=" << tdata2->i2_ <<
		", numAcks=" << tdata2->numAcks_ << endl;
	*/
	// Check results
	
	assert( tdata1->s_ == " goodbye tdata1" );
	assert( tdata2->s_ == " hello TDATA2" );
	assert( tdata1->i1_ == 5001  );
	assert( tdata1->i2_ == 6002  );
	assert( tdata2->i1_ == 1005  );
	assert( tdata2->i2_ == 2006  );
	assert( tdata1->numAcks_ == 4  ); // not good.
	assert( tdata2->numAcks_ == 0  );
	
	t1.destroy();
	t2.destroy();

	cout << "." << flush;
}

void testAsync( )
{
	showFields();
	insertIntoQ();
	testSendMsg();
	testCreateMsg();
	testSet();
	testGet();
	testSetGet();
	testSetGetDouble();
	testSetGetSynapse();
	testSetGetVec();
	testSendSpike();
	testSparseMatrix();
	testSparseMatrixBalance();
	testSparseMsg();
	testUpValue();
	testSharedMsg();
}
