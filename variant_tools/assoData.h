/*
 *  $File: assoData.h $
 *  $LastChangedDate$
 *  $Rev$
 *
 *  This file is part of variant_tools, a software application to annotate,
 *  summarize, and filter variants for next-gen sequencing ananlysis.
 *  Please visit http://varianttools.sourceforge.net for details.
 *
 *  Copyright (C) 2011 Gao Wang (wangow@gmail.com) and Bo Peng (bpeng@mdanderson.org)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef _ASSODATA_H
#define _ASSODATA_H


#include <vector>
typedef std::vector<double> vectorf;
typedef std::vector<std::vector<double> > matrixf;
typedef std::vector<int> vectori;
typedef std::vector<std::vector<int> > matrixi;
#include <cassert>
#include <numeric>
#include <algorithm>
#include <functional>
#include <iostream>
#include <limits>
#include "assoConfig.h"
#include "utils.h"
#include "gsl/gsl_cdf.h"
#include "gsl/gsl_randist.h"


namespace vtools {

class AssoData
{
public:
	AssoData() :
		m_phenotype(0), m_genotype(0), m_maf(0), m_X(0),
		m_statistic(0), m_se(0), m_pval(0), m_C(0), m_ncovar(0),
		m_xbar(0.0), m_ybar(0.0), m_ncases(0),
		m_nctrls(0), m_model()
	{
	}


	~AssoData(){}

	// make a copy of the data
	virtual AssoData * clone() const
	{
		return new AssoData(*this);
	}


	// set data
	double setGenotype(const matrixf & g)
	{
		//codings are 0, 1, 2, -9 and U(0,1) number for "expected" genotype
		m_genotype = g;
		return 0.0;
	}

	double setX(const vectorf & g)
	{
		m_X = g;
		return 0.0;
	}

	double setPhenotype(const vectorf & p)
	{
		m_phenotype = p;
		// set phenotype statistics
		m_ybar = std::accumulate(m_phenotype.begin(), m_phenotype.end(), 0.0);
		m_ybar /= (1.0 * m_phenotype.size());
		//m_ncases = (unsigned) std::count_if(m_phenotype.begin(), m_phenotype.end(),
		//    std::bind2nd(std::equal_to<double>(),1.0));
		//m_nctrls = (unsigned) std::count_if(m_phenotype.begin(), m_phenotype.end(),
		//    std::bind2nd(std::equal_to<double>(),0.0));
        m_statistic.resize(1);
        m_se.resize(1);
        m_pval.resize(m_statistic.size());
		return m_ybar;
	}


	double setPhenotype(const vectorf & p, const matrixf & c)
	{
		m_phenotype = p;
		m_C = c;
		m_ncovar = c.size() - 1;
		vectorf one(p.size());
		std::fill(one.begin(), one.end(), 1.0);
		m_C.push_back(one);
		m_model.clear();
		m_model.setY(m_phenotype);
		m_model.setX(m_C);
		// set phenotype statistics
		m_ybar = std::accumulate(m_phenotype.begin(), m_phenotype.end(), 0.0);
		m_ybar /= (1.0 * m_phenotype.size());
		//m_ncases = (unsigned) std::count_if(m_phenotype.begin(), m_phenotype.end(),
		//    std::bind2nd(std::equal_to<double>(),1.0));
		//m_nctrls = (unsigned) std::count_if(m_phenotype.begin(), m_phenotype.end(),
		//    std::bind2nd(std::equal_to<double>(),0.0));
        m_statistic.resize((m_ncovar+1));
        m_se.resize((m_ncovar+1));
        m_pval.resize(m_statistic.size());
		return m_ybar;
	}


	void setMaf(const vectorf & maf)
	{
		//get this field directly from the variant table
		m_maf = maf;
	}


	void setMaf()
	{
		//sample maf from data
		//is problematic for variants on male chrX
		//but should be Ok if only use the relative mafs (e.g., weightings)

		m_maf.resize(m_genotype.front().size());
		std::fill(m_maf.begin(), m_maf.end(), 0.0);
		vectorf valid_all = m_maf, valid_alt = m_maf;

		for (size_t j = 0; j < m_maf.size(); ++j) {
			unsigned cnt1 = 0, cnt2 = 0;
			// calc maf and loci counts for site j
			for (size_t i = 0; i < m_genotype.size(); ++i) {
				// genotype not missing
				if (!(m_genotype[i][j] < 0.0)) {
					valid_all[j] += 1.0;
					if (m_genotype[i][j] > 0.0) {
						if (fEqual(m_genotype[i][j], 1.0)) cnt1 += 1;
						if (fEqual(m_genotype[i][j], 2.0)) cnt2 += 1;
						valid_alt[j] += 1.0;
						m_maf[j] += m_genotype[i][j];
					}
				}
			}
			//
			//
			// RECODING
			// FIXME : currently it is an awkward re-coding approach,
			// will recode sites where #alt>#ref alleles,
			// assumes either excess of homo (2222200) only or heter (1111100 for male chrX) only,
			// not like (1222200) or (2111100), which, if happens, will check if there are more 1's than 2's
			// if #1>#2 (2111100) then have to put as (0000011)
			// otherwise (1222200) should actually be (1000022)
			//
			if (valid_all[j] < 2.0 * valid_alt[j]) {
				// recoding is needed
				m_maf[j] = 0.0;
				// recode and re-calculate maf
				for (size_t i = 0; i < m_genotype.size(); ++i) {
					// genotype not missing
					if (!(m_genotype[i][j] < 0.0)) {
						if (cnt1 >= cnt2) {
							// recode (1111100) to (0000011)
							// force cases of (2111100) to be (0000011)
							m_genotype[i][j] = (m_genotype[i][j] > 0.0) ? 0.0 : 1.0;
						} else{
							// for (2222200) or (1222200) just flip the coding
							m_genotype[i][j] = 2.0 - m_genotype[i][j];
						}
						if (m_genotype[i][j] > 0.0) {
							m_maf[j] += m_genotype[i][j];
						}
					}
				}
			}
			//
			// count maf, will be incorrect for male chrX,
			// but will not matter for our purpose
			//
			if (valid_all[j] > 0.0) {
				m_maf[j] = m_maf[j] / (valid_all[j] * 2.0);
			}
		}
		/*
		   m_maf = std::accumulate(m_genotype.begin() + 1, m_genotype.end(),
		    m_genotype.front(), vplus);
		   std::transform(m_maf.begin(), m_maf.end(), m_maf.begin(),
		    std::bind2nd(std::divides<double>(), 2.0*m_genotype.size()));
		 */
	}


	void setMafWeight()
	{
		// compute w = 1 / sqrt(p*(1-p))
		if (m_maf.size() == 0) {
			throw RuntimeError("MAF has not been calculated. Please calculate MAF prior to calculating weights.");
		}
		//
		m_weight.clear();
		for (size_t i = 0; i < m_maf.size(); ++i) {
			if (fEqual(m_maf[i], 0.0) || fEqual(m_maf[i], 1.0)) {
				m_weight.push_back(0.0);
			} else{
				m_weight.push_back(1.0 / sqrt(m_maf[i] * (1.0 - m_maf[i])));
			}
		}
	}


	void setMafWeight(const std::vector<size_t> & idx)
	{
		// FIXME weight by maf from selected samples (ctrls, low QT samples, etc)
	}


	double meanOfX()
	{
		m_xbar = std::accumulate(m_X.begin(), m_X.end(), 0.0);
		m_xbar /= (1.0 * m_X.size());
		return m_xbar;
	}


	// return some information
	vectorf phenotype()
	{
		return m_phenotype;
	}


	vectorf genotype()
	{
		return m_X;
	}


	matrixf raw_genotype()
	{
		return m_genotype;
	}


	matrixf covariates()
	{
		return m_model.getX();
	}


	vectorf maf()
	{
		return m_maf;
	}


	unsigned covarcounts()
	{
		return m_ncovar;
	}


	unsigned samplecounts()
	{
		return m_phenotype.size();
	}


	vectorf pvalue()
	{
		return m_pval;
	}


	double setPvalue(vectorf pval)
	{
		m_pval = pval;
		return 0.0;
	}

	double setPvalue(double pval)
	{
        m_pval.resize(1);
		m_pval[0] = pval;
		return 0.0;
	}

	double setStatistic(vectorf stat)
	{
		m_statistic = stat;
		return 0.0;
	}

	double setSE(vectorf se)
	{
		m_se = se;
		return 0.0;
	}

	double setStatistic(double stat)
	{
        m_statistic.resize(1);
		m_statistic[0] = stat;
		return 0.0;
	}

	double setSE(double se)
	{
        m_se.resize(1);
		m_se[0] = se;
		return 0.0;
	}

	vectorf statistic()
	{
		return m_statistic;
	}

	vectorf se()
	{
		return m_se;
	}

public:
	void permuteY()
	{
		random_shuffle(m_phenotype.begin(), m_phenotype.end());
	}


	void permuteRawX()
	{
		random_shuffle(m_genotype.begin(), m_genotype.end());
	}


	void permuteX()
	{
		random_shuffle(m_X.begin(), m_X.end());
	}


	// manipulate data
	void sumToX()
    {
        m_X.resize(m_genotype.size());
        std::fill(m_X.begin(), m_X.end(), 0.0);
        for (size_t i = 0; i < m_genotype.size(); ++i) {
            //m_X[i] = std::accumulate(m_genotype[i].begin(), m_genotype[i].end(), 0.0);
            for (size_t j = 0; j < m_genotype[i].size(); ++j) {
                if (m_genotype[i][j] > 0) {
                    m_X[i] += m_genotype[i][j];
                }
            }
        }
    }


	void weightX()
	{
		if (m_weight.size() == 0) {
            return;
        }
		for (size_t i = 0; i < m_genotype.size(); ++i) {
			for (size_t j = 0; j < m_genotype[i].size(); ++j) {
                if (m_genotype[i][j] > 0) {
                    m_genotype[i][j] *= m_weight[j];
                }
			}
		}
	}


	void binToX()
	{
		// binning the data with proper handling of missing genotype
		m_X.resize(m_genotype.size());
		std::fill(m_X.begin(), m_X.end(), 0.0);
		for (size_t i = 0; i < m_genotype.size(); ++i) {
			double pnovar = 1.0;
            for (size_t j = 0; j != m_genotype[i].size(); ++j) {
                if (m_genotype[i][j] >= 1.0) {
                    m_X[i] = 1.0;
                    break;
                } else if (m_genotype[i][j] > 0.0) {
                    pnovar *= (1.0 - m_genotype[i][j]);
                } else ;
            }
			if (pnovar < 1.0 && m_X[i] < 1.0) {
				m_X[i] = 1.0 - pnovar;
			}
		}
	}


	void setSitesByMaf(double upper, double lower)
    {
        if (upper > 1.0) {
            throw ValueError("Minor allele frequency value should not exceed 1");
        }
        if (lower < 0.0) {
            throw ValueError("Minor allele frequency should be a positive value");
        }

        if (fEqual(upper, 1.0) && fEqual(lower, 0.0)) return;

        for (size_t j = 0; j != m_maf.size(); ++j) {
            if (m_maf[j] <= lower || m_maf[j] > upper) {
                m_maf.erase(m_maf.begin() + j);
                for (size_t i = 0; i < m_genotype.size(); ++i) {
                    m_genotype[i].erase(m_genotype[i].begin() + j);
                }
                --j;
            }
        }
        return;
    }


	void simpleLinear()
	{
		// simple linear regression score test
		//!- See page 23 and 41 of Kutner's Applied Linear Stat. Model, 5th ed.
		//
		if (m_X.size() != m_phenotype.size()) {
			throw ValueError("Genotype/Phenotype length not equal!");
		}
		double numerator = 0.0, denominator = 0.0, ysigma = 0.0;
		for (size_t i = 0; i != m_X.size(); ++i) {
			numerator += (m_X[i] - m_xbar) * m_phenotype[i];
			denominator += pow(m_X[i] - m_xbar, 2.0);
		}

		if (!fEqual(numerator, 0.0)) {
			//!- Compute MSE and V[\hat{beta}]
			//!- V[\hat{beta}] = MSE / denominator
			double b1 = numerator / denominator;
			double b0 = m_ybar - b1 * m_xbar;

			//SSE
			for (size_t i = 0; i != m_X.size(); ++i) {
				ysigma += pow(m_phenotype[i] - (b0 + b1 * m_X[i]), 2.0);
			}
			double varb = ysigma / (m_phenotype.size() - 2.0) / denominator;
			m_statistic[0] = b1 / sqrt(varb);
			m_se[0] = sqrt(varb);
		} else {
            m_statistic[0] = 0.0;
            m_se[0] = std::numeric_limits<double>::quiet_NaN();
        }
	}


	void simpleLogit()
    {
        //!- Score test implementation for logistic regression model logit(p) = b0 + b1x
        //!- labnotes vol.2 page 3
        //!- input phenotypes have to be binary values 0 or 1
        if (m_X.size() != m_phenotype.size()) {
            throw ValueError("Genotype/Phenotype length not equal!");
        }
        //double ebo = (1.0 * n1) / (1.0 * (m_phenotype.size()-n1));
        //double bo = log(ebo);

        double po = (1.0 * m_ncases) / (1.0 * m_phenotype.size());
        double ss = 0.0;
        // the score
        for (size_t i = 0; i != m_X.size(); ++i) {
            ss += (m_X[i] - m_xbar) * (m_phenotype[i] - po);
        }
        double vm1 = 0.0;
        // variance of score, under the null
        for (size_t i = 0; i != m_X.size(); ++i) {
            vm1 += (m_X[i] - m_xbar) * (m_X[i] - m_xbar) * po * (1.0 - po);
        }

        ss = ss / sqrt(vm1);

        //!-FIXME: (not sure why this happens)
        //!- w/ rounding to 0 I get strange number such as 3.72397e-35
        //!- this would lead to type I error problem
        fRound(ss, 0.0001);
        m_statistic[0] = ss;
        m_se[0] = sqrt(vm1);
    }


	void multipleLinear()
    {
        //!- multiple linear regression parameter estimate
        //!- BETA= (X'X)^{-1}X'Y => (X'X)BETA = X'Y
        //!- Solve the system via gsl_linalg_SV_solve()
        if (m_X.size() != m_phenotype.size()) {
            throw ValueError("Genotype/Phenotype length not equal!");
        }
        // reset phenotype data
        m_model.replaceCol(m_phenotype, 0);
        // reset genotype data
        m_model.replaceCol(m_X, m_C.size() - 1);
        m_model.fit();
        vectorf beta = m_model.getBeta();
        vectorf seb = m_model.getSEBeta();
        m_statistic[0] = beta.back() / seb.back();
        m_se[0] = seb.back();
        for (unsigned i = 1; i < beta.size() - 1; ++i) {
            m_statistic[i] = beta[i]/seb[i];
            m_se[i] = seb[i];
        }
    }


	void gaussianP(unsigned sided = 1)
    {
        if (sided == 1) {
            for (unsigned i = 0; i < m_statistic.size(); ++i) {
                m_pval[i] = gsl_cdf_ugaussian_Q(m_statistic[i]);
            }
        } else if (sided == 2) {
            for (unsigned i = 0; i < m_statistic.size(); ++i) {
                m_pval[i] = gsl_cdf_chisq_Q(m_statistic[i] * m_statistic[i], 1.0);
            }
        } else {
            throw ValueError("Alternative hypothesis should be one-sided (1) or two-sided (2)");
        }
    }


	void studentP(unsigned sided = 1)
    {
        // df = n - p where p = #covariates + 1 (for beta1) + 1 (for beta0) = m_ncovar+2
        if (sided == 1) {
            for (unsigned i = 0; i < m_statistic.size(); ++i) {
                m_pval[i] = gsl_cdf_tdist_Q(m_statistic[i], m_phenotype.size() - (m_ncovar + 2.0));
            }
        } else if (sided == 2) {
            for (unsigned i = 0; i < m_statistic.size(); ++i) {
                double p = gsl_cdf_tdist_Q(m_statistic[i], m_phenotype.size() - (m_ncovar + 2.0));
                m_pval[i] = fmin(p, 1.0 - p) * 2.0;
            }
        } else  {
            throw ValueError("Alternative hypothesis should be one-sided (1) or two-sided (2)");
        }
    }


private:
	/// raw phenotype and gneotype data
	vectorf m_phenotype;
	matrixf m_genotype;

	// covariates
	matrixf m_C;
	unsigned m_ncovar;

	// observed minor allele frequencies
	vectorf m_maf;
	/// translated genotype
	vectorf m_X;

	// weights
	vectorf m_weight;

	double m_xbar;
	double m_ybar;
	unsigned m_ncases;
	unsigned m_nctrls;
	vectorf m_pval;
	vectorf m_statistic;
    vectorf m_se;
	LinearM m_model;
};

}
#endif
