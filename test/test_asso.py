#!/usr/bin/env python

import sys
try:
    import variant_tools.assoTests as t
except:
    pass

if __name__ == '__main__':
    data = t.AssoData()
    data.setPhenotype([1, 2, 1, 2])
    data.setGenotype([
        [1, 2, 3, 4],
        [2, 1, 2, 1],
        [1, 2, 2, 2],
        [1, 2, 3, 2]
        ])

    a = t.SumToX()
    a.apply(data)
    print data.phenotype()
    print data.genotype()

#
    p = t.PhenoPermutator(100, [t.SomeTest()])
    print p.permute(data)
