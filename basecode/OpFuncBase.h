/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2009 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _OPFUNCBASE_H
#define _OPFUNCBASE_H

extern const unsigned char MooseSendHop;
extern const unsigned char MooseSetHop;
extern const unsigned char MooseGetHop;
extern const unsigned char MooseReturnHop;
extern const unsigned char MooseTestHop;

class HopIndex
{
	public:
		HopIndex( unsigned short bindIndex, 
				unsigned char hopType = MooseSendHop)
				: bindIndex_( bindIndex ),
				hopType_( hopType )
		{;}

		unsigned short bindIndex() const {
			return bindIndex_;
		}

		unsigned char hopType() const {
			return hopType_;
		}
	private:
		unsigned short bindIndex_;
		unsigned char hopType_;
};

class OpFunc
{
	public:
		OpFunc();

		virtual ~OpFunc()
		{;}
		virtual bool checkFinfo( const Finfo* s) const = 0;

		virtual string rttiType() const = 0;

		virtual const OpFunc* makeHopFunc( HopIndex hopIndex) const =0;

		/// Executes the OpFunc by converting args.
		virtual void opBuffer( const Eref& e, double* buf ) const = 0;

		static const OpFunc* lookop( unsigned int opIndex );

		unsigned int opIndex() const {
			return opIndex_;
		}
	private:
		unsigned int opIndex_;
		static vector< OpFunc* >& ops();
};

class OpFunc0Base: public OpFunc
{
	public:
		bool checkFinfo( const Finfo* s ) const {
			return dynamic_cast< const SrcFinfo0* >( s );
		}

		virtual void op( const Eref& e ) const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		void opBuffer( const Eref& e, double* buf ) const;

		string rttiType() const {
			return "void";
		}
};

template< class A > class OpFunc1Base: public OpFunc
{
	public:

		bool checkFinfo( const Finfo* s ) const {
			return dynamic_cast< const SrcFinfo1< A >* >( s );
		}

		virtual void op( const Eref& e, A arg ) const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		void opBuffer( const Eref& e, double* buf ) const {
			op( e, Conv< A >::buf2val( &buf ) );
		}

		string rttiType() const {
			return Conv< A >::rttiType();
		}
};

template< class A1, class A2 > class OpFunc2Base: public OpFunc
{
	public:
		bool checkFinfo( const Finfo* s ) const {
			return dynamic_cast< const SrcFinfo2< A1, A2 >* >( s );
		}

		virtual void op( const Eref& e, A1 arg1, A2 arg2 ) 
				const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		void opBuffer( const Eref& e, double* buf ) const {
			const A1& arg1 = Conv< A1 >::buf2val( &buf );
			op( e, 		
				arg1, Conv< A2 >::buf2val( &buf ) );
		}

		string rttiType() const {
			return Conv< A1 >::rttiType() + "," + Conv< A2 >::rttiType(); 
		}
};

template< class A1, class A2, class A3 > class OpFunc3Base: 
	public OpFunc
{
	public:
		bool checkFinfo( const Finfo* s ) const {
			return dynamic_cast< const SrcFinfo3< A1, A2, A3 >* >( s );
		}

		virtual void op( const Eref& e, A1 arg1, A2 arg2, A3 arg3 ) 
				const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		void opBuffer( const Eref& e, double* buf ) const {
			const A1& arg1 = Conv< A1 >::buf2val( &buf );
			const A2& arg2 = Conv< A2 >::buf2val( &buf );
			op( e, 		
				arg1, arg2, Conv< A3 >::buf2val( &buf ) );
		}

		string rttiType() const {
			return Conv< A1 >::rttiType() + "," + Conv< A2 >::rttiType() +
				"," + Conv< A3 >::rttiType();
		}
};

template< class A1, class A2, class A3, class A4 > 
	class OpFunc4Base: public OpFunc
{
	public:
		bool checkFinfo( const Finfo* s ) const {
			return dynamic_cast< const SrcFinfo4< A1, A2, A3, A4 >* >( s );
		}

		virtual void op( const Eref& e, 
						A1 arg1, A2 arg2, A3 arg3, A4 arg4 ) const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		void opBuffer( const Eref& e, double* buf ) const {
			const A1& arg1 = Conv< A1 >::buf2val( &buf );
			const A2& arg2 = Conv< A2 >::buf2val( &buf );
			const A3& arg3 = Conv< A3 >::buf2val( &buf );
			op( e,
				arg1, arg2, arg3, Conv< A4 >::buf2val( &buf ) );
		}

		string rttiType() const {
			return Conv< A1 >::rttiType() + "," + Conv< A2 >::rttiType() +
				"," + Conv<A3>::rttiType() + "," + Conv<A4>::rttiType();
		}
};

template< class A1, class A2, class A3, class A4, class A5 > 
	class OpFunc5Base: public OpFunc
{
	public:
		bool checkFinfo( const Finfo* s ) const {
			return dynamic_cast< const SrcFinfo5< A1, A2, A3, A4, A5 >* >( s );
		}

		virtual void op( const Eref& e, 
				A1 arg1, A2 arg2, A3 arg3, A4 arg4, A5 arg5 ) const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		void opBuffer( const Eref& e, double* buf ) const {
			const A1& arg1 = Conv< A1 >::buf2val( &buf );
			const A2& arg2 = Conv< A2 >::buf2val( &buf );
			const A3& arg3 = Conv< A3 >::buf2val( &buf );
			const A4& arg4 = Conv< A4 >::buf2val( &buf );
			op( e,
				arg1, arg2, arg3, arg4, Conv< A5 >::buf2val( &buf ) );
		}

		string rttiType() const {
			return Conv< A1 >::rttiType() + "," + Conv< A2 >::rttiType() +
				"," + Conv<A3>::rttiType() + "," + Conv<A4>::rttiType() +
				"," + Conv<A5>::rttiType();
		}
};

template< class A1, class A2, class A3, class A4, class A5, class A6 > 
		class OpFunc6Base: public OpFunc
{
	public:
		bool checkFinfo( const Finfo* s ) const {
			return dynamic_cast< const SrcFinfo6< A1, A2, A3, A4, A5, A6 >* >( s );
		}

		virtual void op( const Eref& e, A1 arg1, A2 arg2, A3 arg3, A4 arg4, 
						A5 arg5, A6 arg6 ) const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		void opBuffer( const Eref& e, double* buf ) const {
			const A1& arg1 = Conv< A1 >::buf2val( &buf );
			const A2& arg2 = Conv< A2 >::buf2val( &buf );
			const A3& arg3 = Conv< A3 >::buf2val( &buf );
			const A4& arg4 = Conv< A4 >::buf2val( &buf );
			const A5& arg5 = Conv< A5 >::buf2val( &buf );
			op( e, 		
				arg1, arg2, arg3, arg4, arg5, Conv< A6 >::buf2val( &buf ) );
		}

		string rttiType() const {
			return Conv< A1 >::rttiType() + "," + Conv< A2 >::rttiType() +
				"," + Conv<A3>::rttiType() + "," + Conv<A4>::rttiType() +
				"," + Conv<A5>::rttiType() + "," + Conv<A6>::rttiType();
		}
};

/**
 * This is the base class for all GetOpFuncs. 
 */
template< class A > class GetOpFuncBase: public OpFunc1Base< A* >
{
	public: 
			/*
		bool checkFinfo( const Finfo* s ) const {
			return ( dynamic_cast< const SrcFinfo1< A >* >( s )
			|| dynamic_cast< const SrcFinfo1< FuncId >* >( s ) );
		}
		*/

		virtual A returnOp( const Eref& e ) const = 0;

		// This returns an OpFunc1< A* > so we can pass back the arg A
		const OpFunc* makeHopFunc( HopIndex hopIndex) const;

		// This is called on the target node when a remoteGet happens.
		// It needs to do the 'get' function and stuff the data into the
		// buffer for sending back.
		void opBuffer( const Eref& e, double* buf ) const {
			A ret = returnOp( e );
			buf[0] = Conv<A>::size( ret );
			buf++;
			Conv< A >::val2buf( ret, &buf );
		}

		/*
		string rttiType() const {
			return Conv< A >::rttiType();
		}
		*/
};
/*
template< class A > class GetOpFuncBase: public OpFunc
{
	public: 
		bool checkFinfo( const Finfo* s ) const {
			return ( dynamic_cast< const SrcFinfo1< A >* >( s )
			|| dynamic_cast< const SrcFinfo1< FuncId >* >( s ) );
		}

		virtual void op( const Eref& e, ObjId recipient, FuncId fid ) 
				const = 0;

		virtual A returnOp( const Eref& e ) const = 0;

		string rttiType() const {
			return Conv< A >::rttiType();
		}
};
*/

/**
 * This is the base class for all LookupGetOpFuncs. 
 */
template< class L, class A > class LookupGetOpFuncBase: public OpFunc
{
	public: 
		bool checkFinfo( const Finfo* s ) const {
			return ( dynamic_cast< const SrcFinfo1< A >* >( s )
			|| dynamic_cast< const SrcFinfo2< FuncId, L >* >( s ) );
		}

		virtual void op( const Eref& e, L index, 
						ObjId recipient, FuncId fid ) const = 0;

		virtual A returnOp( const Eref& e, const L& index ) const = 0;

		const OpFunc* makeHopFunc( HopIndex hopIndex) const 
		{
			// Perhaps later we can put in something for x-node gets.
			return 0;
		}

		void opBuffer( const Eref& e, double* buf ) const {
				// Later figure out how to handle.
		}

		string rttiType() const {
			return Conv< A >::rttiType();
		}
};

#if 0
// Should I template these off an integer for generating a family?
class OpFuncDummy: public OpFunc
{
	public:
		OpFuncDummy();
		bool checkFinfo( const Finfo* s) const;
		bool checkSet( const SetGet* s) const;

		bool strSet( const Eref& tgt, 
			const string& field, const string& arg ) const;

		void op( const Eref& e, const Qinfo* q, const double* buf ) const;
		string rttiType() const;
};

/**
 * This class is used in the forall call to extract a list of all DataIds
 * on the DataHandler.
 */
class DataIdExtractor: public OpFuncDummy
{
	public:
		DataIdExtractor( vector< DataId >* vec )
			: vec_( vec )
		{;}
		void op( const Eref& e, const Qinfo* q, const double* buf) const
		{
			vec_->push_back( e.index() );
		}
	private:
		vector< DataId >* vec_;
};
#endif

#endif // _OPFUNCBASE_H
