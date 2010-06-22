/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _STOICH_H
#define _STOICH_H

class Stoich: public Data
{
	public: 
		Stoich();

		//////////////////////////////////////////////////////////////////
		// Field assignment stuff
		//////////////////////////////////////////////////////////////////

		void setOneWay( bool v );
		bool getOneWay() const;

		void setPath( string v );
		string getPath() const;

		//////////////////////////////////////////////////////////////////
		// Dest funcs
		//////////////////////////////////////////////////////////////////

		void process( const ProcInfo* p, const Eref& e );
		void eprocess( Eref e, const Qinfo* q, ProcInfo* p );
		void reinit( Eref e, const Qinfo* q, ProcInfo* p );

		static const Cinfo* initCinfo();
	protected:
		bool useOneWay_;
		string path_;
		vector< double > S_;
		vector< double > Sinit_;
		vector< double > v_;
		vector< RateTerm* > rates_;
		KinSparseMatrix N_;

		/**
		 * Maps Ids to objects in the S_ and RateTerm vectors.
		 * There will be holes in this map, but look up is very fast.
		 * The calling Id must know what it wants to find: all it
		 * gets back is an integer.
		 * The alternative is to have multiple maps, but that is slower.
		 * Assume no arrays. Each Mol/reac etc must be a unique
		 * Element. Later we'll deal with diffusion.
		 */
		vector< unsigned int > objMap_;
		/**
		 * Minor efficiency: We will usually have a set of objects that are
		 * nearly contiguous in the map. May as well start with the first of
		 * them.
		 */
		unsigned int objMapStart_;
};

#endif	// _STOICH_H
