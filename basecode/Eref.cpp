#include "header.h"

Eref::Eref( Element* e, unsigned int index )
			: e_( e ), i_( index )
{
	;
}

double Eref::sumBuf( Slot slot )
{
	return e_->sumBuf( slot, i_ );
}

double Eref::prdBuf( Slot slot, double v )
{
	return e_->prdBuf( slot, i_, v );
}

double Eref::oneBuf( Slot slot )
{
	return e_->oneBuf( slot, i_ );
}

double* Eref::getBufPtr( Slot slot )
{
	return e_->getBufPtr( slot, i_ );
}
