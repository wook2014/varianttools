/* =====================================================================================
// 
//  This is a small C and Python library for reading Plink genotype files,
//  written by Mattias Franberg, version 0.2.2 
//  
//  https://bitbucket.org/mattias_franberg/libplinkio
//
//  This software is not licensed or copyrighted. The varianttools developers
//  have been contacting its author and will include the license information when we
//  hear from the author, or replace it with alternative implementation if the author
//  requests for a removal.
// 
 ===================================================================================== */



#ifndef __FAM_PARSE_H__
#define __FAM_PARSE_H__

#include <stdio.h>


#include <fam.h>
#include <status.h>

/**
 * Parses the samples and points the given sample array to a
 * the memory that contains them, and writes back the number
 * of samples.
 *
 * @param fam_fp Fam file.
 * @param sample Parsed samples will be stored here.
 *
 * @return PIO_OK if the samples could be parsed, PIO_ERROR otherwise.
 */
pio_status_t parse_samples(FILE *fam_fp, UT_array *sample);

#endif /* End of __FAM_PARSE_H__ */