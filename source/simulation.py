#!/usr/bin/env python
#
# $File: simulation.py $
# $LastChangedDate: 2014-01-14 10:38:56 -0600 (Tue, 14 Jan 2014) $
# $Rev: 2505 $
#
# This file is part of variant_tools, a software application to annotate,
# summarize, and filter variants for next-gen sequencing ananlysis.
# Please visit http://varianttools.sourceforge.net for details.
#
# Copyright (C) 2011 - 2014 Bo Peng (bpeng@mdanderson.org)
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

import sys, os, re
from .project import Project
from .utils import ProgressBar, DatabaseEngine, delayedAction, env,\
    consolidateFieldName

if sys.version_info.major == 2:
    from ucsctools_py2 import tabixFetch
else:
    from ucsctools_py3 import tabixFetch

import argparse

'''Draft design of the simulation feature

Goals:
1. Simulate 'real' data in the sense that the simulated datasets should have
  chromsome, locations, build, and preferrable realistic regions for 
  exome, etc.

2. Users provide description of samples, preferrably NOT the way to simulate
  it.

3. Results are written as variant tools projects, and should be easy to analyze
  using the subsequent tools. Results can be exported in vcf format.

4. Simulate SNPs, and Indels if model allows.

Design:
1. Use the existing 'show' feature to show available models.
  a. vtools show simulations/models/simu_models
  b. vtools show model SOME_MODEL

2. Use the existing 'snapshot' feature to distribute pre-simulated
  datasets.
  a. vtools show snapshots
  b. vtools admin --load_snapshot

3. Use the existing 'export' feature to export simulated data in vcf format
  a. vtools export --output simulated.vcf

4. Use the existing 'pipeline' feature to simulate complex samples if that
  evolves multiple steps.

Implementations:



Models:
1. Sequencing error model: manipulate existing (simulated) genotype
2. De Nova mutation model: create novel mutations for offspring
3. Phenotype model: create phenotype based on genotypes on selected
   variants.
4. Indel models.
5. Resampling model: easy to implement
6. Coalescane model: ... many stuff with python interface
7. forward-time model: simupop, srv

'''


class NullModel:
    '''A base class that defines a common interface for simulation models'''
    def __init__(self, *method_args):
        '''Args is arbitrary arguments, might need an additional parser to
        parse it'''
        # trait type
        self.trait_type = None
        # group name
        self.gname = None
        self.fields = []
        self.parseArgs(*method_args)
        #

    def parseArgs(self, method_args):
        # this function should never be called.
        raise SystemError('All association tests should define their own parseArgs function')


def getAllModels():
    '''List all simulation models (all classes that subclasses of NullModel) in this module'''
    return sorted([(name, obj) for name, obj in globals().iteritems() \
        if type(obj) == type(NullModel) and issubclass(obj, NullModel) \
            and name not in ('NullModel',)], key=lambda x: x[0])

def simulateArguments(parser):
    parser.add_argument('--regions', nargs='+',
        help='''One or more chromosome regions in the format of chr:start-end
        (e.g. chr21:33,031,597-33,041,570), or Field:Value from a region-based 
        annotation database (e.g. refGene.name2:TRIM2 or refGene_exon.name:NM_000947).
        Chromosome positions are 1-based and are inclusive at both ends so the 
        chromosome region has a length of end-start+1 bp. For the second case,
        multiple chromosomal regions will be selected if the  name matches more
        than one chromosomal regions.''')
    parser.add_argument('--model', 
        help='''Simulation model, which defines the algorithm and default
            parameter to simulate data. A list of model-specific parameters
            could be specified to change the behavior of these models. Commands
            vtools show models and vtools show model MODEL can be used to list
            all available models and details of one model.''')
    

def expandRegions(arg_region, arg_build, proj):
    regions = []
    for region in arg_region:
        try:
            chr, location = region.split(':', 1)
            start, end = location.split('-')
            start = int(start.replace(',', ''))
            end = int(end.replace(',', ''))
            if start == 0 or end == 0:
                raise ValueError('0 is not allowed as starting or ending position')
            if start > end:
                raise ValueError('Ending position {} before starting position {}'
                    .format(end, start))
                regions.append((chr, start, end))
            else:
                regions.append((chr, start, end))
        except Exception as e:
            # this is not a format for chr:start-end, try field:name
            try:
                field, value = region.rsplit(':', 1)
                # what field is this?
                query, fields = consolidateFieldName(proj, 'variant', field, False) 
                # query should be just one of the fields according to things that are passed
                if query.strip() not in fields:
                    raise ValueError('Could not identify field {} from the present project'.format(field))
                # now we have annotation database
                try:
                    annoDB = [x for x in proj.annoDB if x.linked_name.lower() == query.split('.')[0].lower()][0]
                except:
                    raise ValueError('Could not locate annotation database {} in the project'.format(query.split('.')[0]))
                #
                if annoDB.anno_type != 'range':
                    raise ValueError('{} is not linked as a range-based annotation database.'.format(annoDB.linked_name))
                # get the fields?
                chr_field, start_field, end_field = annoDB.build
                #
                # find the regions
                cur = proj.db.cursor()
                cur.execute('SELECT {},{},{} FROM {}.{} WHERE {}="{}"'.format(
                    chr_field, start_field, end_field, annoDB.linked_name, annoDB.name,
                    field.rsplit('.',1)[-1], value))
                for chr, start, end in cur:
                    if start > end:
                        env.logger.warning('Ignoring unrecognized region chr{}:{}-{} from {}'
                            .format(chr, start + 1, end + 1, annoDB.linked_name))
                        continue
                    try:
                        regions.append(('chr{}'.format(chr), int(start) + 1, int(end)))
                    except Exception as e:
                        env.logger.warning('Ignoring unrecognized region chr{}:{}-{} from {}'
                            .format(chr, start + 1, end + 1, annoDB.linked_name))
                if not regions:
                    env.logger.error('No valid chromosomal region is identified for {}'.format(region)) 
            except Exception as e:
                raise ValueError('Incorrect format for chromosomal region {}: {}'.format(region, e))
    # remove duplicates and merge ranges
    regions = list(set(regions))
    while True:
        merged = False
        for i in range(len(regions)):
            for j in range(i + 1, len(regions)):
                r1 = regions[i]
                r2 = regions[j]
                if r1 is None or r2 is None:
                    continue
                if r1[0] == r2[0] and r1[2] >= r2[1] and r1[1] <= r2[2]:
                    env.logger.info('Merging regions {}:{}-{} and {}:{}-{}'
                        .format(r2[0], r2[1], r2[2], r1[0], r1[1], r1[2]))
                    regions[i] = (r1[0], min(r1[1], r2[1]), max(r1[2], r2[2]))
                    regions[j] = None
                    merged = True
        if not merged:
            return sorted([x for x in regions if x is not None])


def extractFromVCF(filenameOrUrl, regions, output=''):
    # This is equivalent to 
    #
    # tabix -h ftp://ftp.1000genomes.ebi.ac.uk/vol1/ftp/release/20100804/
    #     ALL.2of4intersection.20100804.genotypes.vcf.gz 2:39967768-39967768
    #
    #
    tabixFetch(filenameOrUrl, regions)

def simulate(args):
    try:
        with Project(verbosity=args.verbosity) as proj:
            # step 0: 
            # get the model of simulation
            print expandRegions(args.regions, proj.build, proj)
            #extractFromVCF(os.path.expanduser('~/vtools/test/vcf/CEU.vcf.gz'),
            #    ['1:1-100000'])

    except Exception as e:
        env.logger.error(e)
        sys.exit(1)


