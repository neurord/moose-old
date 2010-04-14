/**********************************************************************
** This program is part of 'MOOSE', the
** Messaging Object Oriented Simulation Environment.
**           Copyright (C) 2003-2010 Upinder S. Bhalla. and NCBS
** It is made available under the terms of the
** GNU Lesser General Public License version 2.1
** See the file COPYING.LIB for the full notice.
**********************************************************************/

#ifndef _ONE_DIMENSION_DATA_H
#define _ONE_DIMENSION_DATA_H

/**
 * This class manages the data part of Elements. It handles a one-
 * dimensional array.
 */
class OneDimensionData: public DataHandler
{
	public:
		OneDimensionData( const DinfoBase* dinfo );

		~OneDimensionData();

		/**
		 * Returns the data on the specified index.
		 */
		char* data( DataId index ) const;

		/**
		 * Returns the data at one level up of indexing. In this case it
		 * just returns the data on the specified index.
		 */
		char* data1( DataId index ) const;

		/**
		 * Returns the number of data entries.
		 */
		unsigned int numData() const {
			return size_;
		}

		/**
		 * Returns the number of data entries at index 1.
		 * For regular Elements this is identical to numData.
		 */
		unsigned int numData1() const {
			return size_;
		}

		/**
		 * Returns the number of data entries at index 2, if present.
		 * For regular Elements and 1-D arrays this is always 1.
		 */
		 unsigned int numData2( unsigned int index1 ) const {
		 	return 1;
		 }

		/**
		 * Returns the number of dimensions of the data.
		 */
		unsigned int numDimensions() const {
			return 1;
		}

		/**
		 * Assigns the size for the data dimension.
		 */
		void setNumData1( unsigned int size );

		/**
		 * Assigns the sizes of all array field entries at once.
		 * Ignore in this case, as there are none.
		 */
		void setNumData2( const vector< unsigned int >& sizes );

		/**
		 * Looks up the sizes of all array field entries at once.
		 * Ignore in this case, as there are no array fields.
		 */
		void getNumData2( vector< unsigned int >& sizes ) const;

		/**
		 * Returns true if the node decomposition has the data on the
		 * current node
		 */
		bool isDataHere( DataId index ) const;

		bool isAllocated() const;

		void allocate();

	private:
		char* data_;
		unsigned int size_;	// Number of data entries in the whole array
		unsigned int start_;	// Starting index of data, used in MPI.
		unsigned int end_;	// Starting index of data, used in MPI.
};

#endif	// _ONE_DIMENSION_DATA_H
