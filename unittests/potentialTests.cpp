#include "potentialTests.h"
#include "projectedPotential.h"
#include "PRISM01_calcPotential.h"
#include <boost/test/unit_test.hpp>
#include "ArrayND.h"
#include <iostream>
#include "kirkland_params.h"
#include <vector>
#include "params.h"
#include "atom.h"

namespace Prismatic{

static const PRISMATIC_FLOAT_PRECISION pi = std::acos(-1);
PRISMATIC_FLOAT_PRECISION a0 = 0.529; //bohr radius
PRISMATIC_FLOAT_PRECISION e = 14.4; //electron charge in Volt-Angstoms
PRISMATIC_FLOAT_PRECISION term1 =  2*pi*pi*a0*e;
PRISMATIC_FLOAT_PRECISION term2 = 2*pow(pi,5.0/2.0)*a0*e;
    
PRISMATIC_FLOAT_PRECISION checkFaces(Array3D<PRISMATIC_FLOAT_PRECISION> &pot)
{
    
    size_t xFace[] = {0, pot.get_dimi()-1};
    size_t yFace[] = {0, pot.get_dimj()-1};
    size_t zFace[] = {0, pot.get_dimk()-1};
    PRISMATIC_FLOAT_PRECISION errSum = 0;
    //+-x
    for(auto i = 0; i < 2; i++)
    {
        for(auto j = 0; j < pot.get_dimj(); j++)
        {
            for(auto k = 0; k < pot.get_dimk(); k++)
            {
                errSum += pot.at(k,j,xFace[i]);
            }
        }
    }

    //+-y
    for(auto j = 0; j < 2; j++)
    {
        for(auto i = 0; i < pot.get_dimi(); i++)
        {
            for(auto k = 0; k < pot.get_dimk(); k++)
            {
                errSum += pot.at(k,yFace[j],i);
            }
        }
    }

    //+-z
    for(auto k = 0; k < 2; k++)
    {
        for(auto i = 0; i < pot.get_dimi(); i++)
        {
            for(auto j = 0; j < pot.get_dimj(); j++)
            {
                errSum += pot.at(zFace[k],j,i);
            }
        }
    }
    return errSum;
};

Array3D<PRISMATIC_FLOAT_PRECISION> generateInterpolationIdentity(size_t xSize,size_t ySize, size_t zSize,
                                                                            PRISMATIC_FLOAT_PRECISION dx,
                                                                            PRISMATIC_FLOAT_PRECISION dy,
                                                                            PRISMATIC_FLOAT_PRECISION dz)
{
    Array3D<PRISMATIC_FLOAT_PRECISION> identity = zeros_ND<3,PRISMATIC_FLOAT_PRECISION>({{zSize,ySize,xSize}});

    std::vector<size_t> xind = (dx < 0) ? std::vector<size_t>{0, identity.get_dimi()-2} : std::vector<size_t>{1, identity.get_dimi()-1};
    std::vector<size_t> yind = (dy < 0) ? std::vector<size_t>{0, identity.get_dimj()-2} : std::vector<size_t>{1, identity.get_dimj()-1};
    std::vector<size_t> zind = (dz < 0) ? std::vector<size_t>{0, identity.get_dimk()-2} : std::vector<size_t>{1, identity.get_dimk()-1};

    PRISMATIC_FLOAT_PRECISION dxm = (dx == 0) ? 1 : dx;
    PRISMATIC_FLOAT_PRECISION dym = (dy == 0) ? 1 : dy;
    PRISMATIC_FLOAT_PRECISION dzm = (dz == 0) ? 1 : dz;

    for(auto k = zind[0]; k <= zind[1]; k++)
    {
        for(auto j = yind[0]; j <= yind[1]; j++)
        {
            for(auto i = xind[0]; i <= xind[1]; i++)
            {
                identity.at(k,j,i) = 1;
                if(i == xind[0]) identity.at(k,j,i)*=std::abs(dxm);
                if(i == xind[1]) identity.at(k,j,i)*=std::abs(dx);

                if(j == yind[0]) identity.at(k,j,i)*=std::abs(dym);
                if(j == yind[1]) identity.at(k,j,i)*=std::abs(dy);
                
                if(k == zind[0]) identity.at(k,j,i)*=std::abs(dzm);
                if(k == zind[1]) identity.at(k,j,i)*=std::abs(dz);
            }
        }
    }

    return identity;
};

void printArray3D(Array3D<PRISMATIC_FLOAT_PRECISION> &arr)
{
    for(auto k = 0; k < arr.get_dimk(); k++)
    {
        for(auto j = 0; j < arr.get_dimj(); j++)
        {
            for(auto i = 0; i < arr.get_dimi(); i++)
            {
                std::cout<<arr.at(k,j,i) << " ";
            }
            std::cout<<std::endl;
        }
        std::cout << "---" << std::endl;
    }
    std::cout << "-----------------------" << std::endl;
};

void addMismatchedArray(Array3D<PRISMATIC_FLOAT_PRECISION> &big,
                        Array3D<PRISMATIC_FLOAT_PRECISION> &small,
                        const size_t &x_offset,
                        const size_t &y_offset,
                        const size_t &z_offset)
{//adds a smaller array to a region of larger area

    for(auto k = 0; k < small.get_dimk(); k++)
    {
        for(auto j = 0; j < small.get_dimj(); j++)
        {
            for(auto i = 0; i < small.get_dimi(); i++)
            {
                if(i+x_offset>= big.get_dimi()) std::cout <<"huhx" << std::endl;
                if(j+y_offset>= big.get_dimj()) std::cout <<"huhy" << std::endl;
                if(k+z_offset>= big.get_dimk()) std::cout <<"huhz" << std::endl;

                big.at(k+z_offset,j+y_offset,i+x_offset) += small.at(k,j,i);
            }
        }
    }

};

BOOST_AUTO_TEST_SUITE(potentialTests);

BOOST_AUTO_TEST_CASE(pot3DFunction)
{
    Array3D<PRISMATIC_FLOAT_PRECISION> radius = ones_ND<3,PRISMATIC_FLOAT_PRECISION>({{10,10,10}});
    Array1D<PRISMATIC_FLOAT_PRECISION> xr = ones_ND<1,PRISMATIC_FLOAT_PRECISION>({{10}});
    Array1D<PRISMATIC_FLOAT_PRECISION> yr = ones_ND<1,PRISMATIC_FLOAT_PRECISION>({{10}});
    Array1D<PRISMATIC_FLOAT_PRECISION> zr = ones_ND<1,PRISMATIC_FLOAT_PRECISION>({{10}});

    //normalize radius to 1 at all points in grid
    xr /= sqrt(3);
    yr /= sqrt(3);
    zr /= sqrt(3);
    
    //seed corner with lower radius so that we can test function with potMin taken into acct
    xr.at(0) = 0.5;
    yr.at(0) = 0.5;
    zr.at(0) = 0.5;

    PRISMATIC_FLOAT_PRECISION r2 = pow(xr.at(0),2) + pow(yr.at(0),2) + pow(zr.at(0),2);
    PRISMATIC_FLOAT_PRECISION r = sqrt(r2);
    
    const size_t Z = 1;
    std::vector<PRISMATIC_FLOAT_PRECISION> parameters;
    parameters.resize(NUM_PARAMETERS);
    for (auto i =0; i < NUM_PARAMETERS; i++) parameters[i] = fparams[(Z-1)*NUM_PARAMETERS + i];
    
    PRISMATIC_FLOAT_PRECISION pot_at_1 = term1*(parameters[0]*exp(-2*pi*sqrt(parameters[1]))
                                        + parameters[2]*exp(-2*pi*sqrt(parameters[3]))
                                        + parameters[4]*exp(-2*pi*sqrt(parameters[5])))
                                + term2*(parameters[6]*pow(parameters[7],-3.0/2.0)*exp(-pi*pi/parameters[7])
                                        + parameters[8]*pow(parameters[9],-3.0/2.0)*exp(-pi*pi/parameters[9])
                                        + parameters[10]*pow(parameters[11],-3.0/2.0)*exp(-pi*pi/parameters[11]));

    PRISMATIC_FLOAT_PRECISION pot_at_0 = term1*(parameters[0]*exp(-2*pi*r*sqrt(parameters[1]))/r
                                        + parameters[2]*exp(-2*pi*r*sqrt(parameters[3]))/r
                                        + parameters[4]*exp(-2*pi*r*sqrt(parameters[5]))/r)
                                + term2*(parameters[6]*pow(parameters[7],-3.0/2.0)*exp(-pi*pi*r2/parameters[7])
                                        + parameters[8]*pow(parameters[9],-3.0/2.0)*exp(-pi*pi*r2/parameters[9])
                                        + parameters[10]*pow(parameters[11],-3.0/2.0)*exp(-pi*pi*r2/parameters[11]));

    PRISMATIC_FLOAT_PRECISION expected = pot_at_0 - pot_at_1; //since potMin only checks 3 points
  
    Array3D<PRISMATIC_FLOAT_PRECISION> pot = kirklandPotential3D(Z, xr, yr, zr);
    PRISMATIC_FLOAT_PRECISION tol = 0.01;
    PRISMATIC_FLOAT_PRECISION error = std::abs(pot.at(0,0,0) - expected);

    BOOST_TEST(error<tol);
};

/*
BOOST_AUTO_TEST_CASE(potMin)
{
    //after potMin correction, potential should be non-negative with all faces of 3D potential prism = 0; 
    //test cubic
    PRISMATIC_FLOAT_PRECISION yleng = 20;
	PRISMATIC_FLOAT_PRECISION xleng = 20;
    PRISMATIC_FLOAT_PRECISION zleng = 20;

	ArrayND<1, std::vector<long>> xvec(std::vector<long>(2 * (size_t)xleng + 1, 0), {{2 * (size_t)xleng + 1}});
	ArrayND<1, std::vector<long>> yvec(std::vector<long>(2 * (size_t)yleng + 1, 0), {{2 * (size_t)yleng + 1}});
	ArrayND<1, std::vector<long>> zvec(std::vector<long>(2 * (size_t)zleng + 1, 0), {{2 * (size_t)zleng + 1}});
	{
		PRISMATIC_FLOAT_PRECISION tmpx = -xleng;
		PRISMATIC_FLOAT_PRECISION tmpy = -yleng;
		PRISMATIC_FLOAT_PRECISION tmpz = -zleng;
		for (auto &i : xvec)
			i = tmpx++;
		for (auto &j : yvec)
			j = tmpy++;
		for (auto &k : zvec)
			k = tmpz++;
	}

    PRISMATIC_FLOAT_PRECISION xPixel = 0.1;
    PRISMATIC_FLOAT_PRECISION yPixel = 0.1;
    PRISMATIC_FLOAT_PRECISION zPixel = 0.1;

    Array1D<PRISMATIC_FLOAT_PRECISION> xr(std::vector<PRISMATIC_FLOAT_PRECISION>(2 * (size_t)xleng + 1, 0), {{2 * (size_t)xleng + 1}});
	Array1D<PRISMATIC_FLOAT_PRECISION> yr(std::vector<PRISMATIC_FLOAT_PRECISION>(2 * (size_t)yleng + 1, 0), {{2 * (size_t)yleng + 1}});
	Array1D<PRISMATIC_FLOAT_PRECISION> zr(std::vector<PRISMATIC_FLOAT_PRECISION>(2 * (size_t)zleng + 1, 0), {{2 * (size_t)zleng + 1}});

	for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * xPixel;
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * yPixel;
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * zPixel;

    const size_t Z = 1; //Hydrogen!
    Array3D<PRISMATIC_FLOAT_PRECISION> pot = kirklandPotential3D(Z, xr, yr, zr);

    PRISMATIC_FLOAT_PRECISION tol = 0.0001;
    //test faces
    PRISMATIC_FLOAT_PRECISION errSum = checkFaces(pot);
    PRISMATIC_FLOAT_PRECISION minVal = pow(2,10); //check for nonnegativity
    for(auto i = 0; i < pot.size(); i++) minVal = (pot[i] < minVal) ? pot[i] : minVal;
    BOOST_TEST(errSum < tol);
    BOOST_TEST(minVal >= 0);

    //test rectangular in 1 direction
    xPixel = 0.05;
    for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * xPixel;
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * yPixel;
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * zPixel;
    pot = kirklandPotential3D(Z, xr, yr, zr);
    errSum = checkFaces(pot);
    minVal = pow(2,10);
    for(auto i = 0; i < pot.size(); i++) minVal = (pot[i] < minVal) ? pot[i] : minVal;
    BOOST_TEST(errSum < tol);
    BOOST_TEST(minVal >= 0);

    xPixel = 0.1;
    yPixel = 0.05;
	for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * xPixel;
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * yPixel;
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * zPixel;
    pot = kirklandPotential3D(Z, xr, yr, zr);
    errSum = checkFaces(pot);
    minVal = pow(2,10);
    for(auto i = 0; i < pot.size(); i++) minVal = (pot[i] < minVal) ? pot[i] : minVal;
    BOOST_TEST(errSum < tol);
    BOOST_TEST(minVal >= 0);

    yPixel = 0.1;
    zPixel = 0.05;
	for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * xPixel;
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * yPixel;
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * zPixel;
    pot = kirklandPotential3D(Z, xr, yr, zr);
    errSum = checkFaces(pot);
    minVal = pow(2,10);
    for(auto i = 0; i < pot.size(); i++) minVal = (pot[i] < minVal) ? pot[i] : minVal;
    BOOST_TEST(errSum < tol);
    BOOST_TEST(minVal >= 0);

    //test rectangulars in 2 directions
    xPixel = 0.05;
    yPixel = 0.05;
    zPixel = 0.1;
	for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * xPixel;
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * yPixel;
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * zPixel;
    pot = kirklandPotential3D(Z, xr, yr, zr);
    errSum = checkFaces(pot);
    minVal = pow(2,10);
    for(auto i = 0; i < pot.size(); i++) minVal = (pot[i] < minVal) ? pot[i] : minVal;
    BOOST_TEST(errSum < tol);
    BOOST_TEST(minVal >= 0);

    xPixel = 0.1;
    zPixel = 0.05;
	for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * xPixel;
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * yPixel;
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * zPixel;
    pot = kirklandPotential3D(Z, xr, yr, zr);
    errSum = checkFaces(pot);
    minVal = pow(2,10);
    for(auto i = 0; i < pot.size(); i++) minVal = (pot[i] < minVal) ? pot[i] : minVal;
    BOOST_TEST(errSum < tol);
    BOOST_TEST(minVal >= 0);

    yPixel = 0.1;
    xPixel = 0.05;
	for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * xPixel;
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * yPixel;
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * zPixel;
    pot = kirklandPotential3D(Z, xr, yr, zr);
    errSum = checkFaces(pot);
    minVal = pow(2,10);
    for(auto i = 0; i < pot.size(); i++) minVal = (pot[i] < minVal) ? pot[i] : minVal;
    BOOST_TEST(errSum < tol);
    BOOST_TEST(minVal >= 0);
};
*/

BOOST_AUTO_TEST_CASE(projPotGeneration)
{
    PRISMATIC_FLOAT_PRECISION tol = 0.001;

    PRISMATIC_FLOAT_PRECISION cellDim = 20.0;
    PRISMATIC_FLOAT_PRECISION potBound = 2.0;
    PRISMATIC_FLOAT_PRECISION sliceThickness = 2.0;
    PRISMATIC_FLOAT_PRECISION sliceThicknessFactor = 4;
    PRISMATIC_FLOAT_PRECISION xPixel = 0.1;
    PRISMATIC_FLOAT_PRECISION yPixel = 0.1;
    
    Array1D<size_t> imageSize = zeros_ND<1, size_t>({{2}});
    imageSize[0] = (round(cellDim/(yPixel*16))*16);
    imageSize[1] = (round(cellDim/(xPixel*16))*16);
    PRISMATIC_FLOAT_PRECISION dzPot = sliceThickness/sliceThicknessFactor;
    std::vector<PRISMATIC_FLOAT_PRECISION> pixelSize;
    pixelSize.push_back(cellDim/imageSize[0]);
    pixelSize.push_back(cellDim/imageSize[1]);
    pixelSize.push_back(dzPot);


	PRISMATIC_FLOAT_PRECISION yleng = std::ceil(potBound/pixelSize[0]);
	PRISMATIC_FLOAT_PRECISION xleng = std::ceil(potBound/pixelSize[1]);
    PRISMATIC_FLOAT_PRECISION zleng = std::ceil(potBound/dzPot);

	ArrayND<1, std::vector<long>> xvec(std::vector<long>(2 * (size_t)xleng + 1, 0), {{2 * (size_t)xleng + 1}});
	ArrayND<1, std::vector<long>> yvec(std::vector<long>(2 * (size_t)yleng + 1, 0), {{2 * (size_t)yleng + 1}});
	ArrayND<1, std::vector<long>> zvec(std::vector<long>(2 * (size_t)zleng + 1, 0), {{2 * (size_t)zleng + 1}});
	{
		PRISMATIC_FLOAT_PRECISION tmpx = -xleng;
		PRISMATIC_FLOAT_PRECISION tmpy = -yleng;
		PRISMATIC_FLOAT_PRECISION tmpz = -zleng;
		for (auto &i : xvec)
			i = tmpx++;
		for (auto &j : yvec)
			j = tmpy++;
		for (auto &k : zvec)
			k = tmpz++;
	}

    Array1D<PRISMATIC_FLOAT_PRECISION> xr(std::vector<PRISMATIC_FLOAT_PRECISION>(2 * (size_t)xleng + 1, 0), {{2 * (size_t)xleng + 1}});
	Array1D<PRISMATIC_FLOAT_PRECISION> yr(std::vector<PRISMATIC_FLOAT_PRECISION>(2 * (size_t)yleng + 1, 0), {{2 * (size_t)yleng + 1}});
	Array1D<PRISMATIC_FLOAT_PRECISION> zr(std::vector<PRISMATIC_FLOAT_PRECISION>(2 * (size_t)zleng + 1, 0), {{2 * (size_t)zleng + 1}});

	for (auto i = 0; i < xr.size(); ++i) xr[i] = (PRISMATIC_FLOAT_PRECISION)xvec[i] * pixelSize[1];
	for (auto j = 0; j < yr.size(); ++j) yr[j] = (PRISMATIC_FLOAT_PRECISION)yvec[j] * pixelSize[0];
	for (auto j = 0; j < zr.size(); ++j) zr[j] = (PRISMATIC_FLOAT_PRECISION)zvec[j] * dzPot;

    //preparing reference potential for single atom potential
    const size_t Z = 79; //Gold!
    Array3D<PRISMATIC_FLOAT_PRECISION> refPot = zeros_ND<3,PRISMATIC_FLOAT_PRECISION>({{zr.size()*2,yr.size()*2,xr.size()*2}});
    Array3D<PRISMATIC_FLOAT_PRECISION> oneH = kirklandPotential3D(Z, xr, yr, zr);
    //loop through quadrants
    size_t offset_x;
    size_t offset_y;
    size_t offset_z;
    for(auto k = 0; k < 2; k++)
    {
        for(auto j = 0; j < 2; j++)
        {
            for(auto i = 0; i < 2; i++)
            {
                offset_x = i*oneH.get_dimi();
                offset_y = j*oneH.get_dimj();
                offset_z = k*oneH.get_dimk();
                addMismatchedArray(refPot, oneH, offset_x, offset_y, offset_z);
            }
        }
    }

    // std::cout << xr.size() << std::endl;
    // PRISMATIC_FLOAT_PRECISION testSum1 = 0.0;
    // PRISMATIC_FLOAT_PRECISION testSum2 = 0.0;
    // for (auto i = 0; i < refPot.size(); i++) testSum1 += refPot[i];
    // for (auto i = 0; i < oneH.size(); i++) testSum2 += oneH[i];
    // std::cout << testSum1 << " " << testSum2 << std::endl;


    //generate reference 
    Array3D<PRISMATIC_FLOAT_PRECISION> interpID;
    PRISMATIC_FLOAT_PRECISION dx, dy, dz;
    Parameters<PRISMATIC_FLOAT_PRECISION> pars;
    pars.meta.sliceThickness = sliceThickness;
    pars.meta.zSampling = sliceThicknessFactor;
    //create list of atoms
    std::vector<atom> atoms;
    atom new_atom;
    std::vector<PRISMATIC_FLOAT_PRECISION> tiledCellDim;
    tiledCellDim.push_back(cellDim);
    tiledCellDim.push_back(cellDim);
    tiledCellDim.push_back(cellDim);
    pars.tiledCellDim = tiledCellDim;

    for(auto i = 0; i < 2; i++)
    {
        for(auto j = 0; j < 2; j++)
        {
            for(auto k = 0; k < 2; k++)
            {
                new_atom.x = (1.0+2.0*i)/4.0;
                new_atom.y = (1.0+2.0*j)/4.0;
                new_atom.z = (1.0+2.0*k)/4.0;
                new_atom.sigma = 0.0800;
                new_atom.species = 1;
                atoms.push_back(new_atom);
            }
        }
    }
    
    pars.atoms = atoms;
    pars.meta.randomSeed = 11111;
    pars.pixelSize = pixelSize;
    pars.imageSize = imageSize;
    generateProjectedPotentials3D(pars, oneH, xvec, yvec, zvec); //H is the only lookup
    //for dx=dy=dz=0, all faces should be zero on potShift equivalent
    Array3D<PRISMATIC_FLOAT_PRECISION> testPot = pars.pot;
    PRISMATIC_FLOAT_PRECISION refPotSum = 0;
    PRISMATIC_FLOAT_PRECISION testPotSum = 0;
    PRISMATIC_FLOAT_PRECISION refPotSum2 = 0;
    for(auto i = 0; i < refPot.size(); i++) refPotSum += refPot[i];
    for(auto i = 0; i < oneH.size(); i++) refPotSum2 += 8*oneH[i];
    for(auto i = 0; i < testPot.size(); i++) testPotSum += testPot[i];

    std::cout << "refPot dims: " << refPot.get_dimi() << " " << refPot.get_dimj() << " " << refPot.get_dimk() << std::endl;
    std::cout << "testPot dims: " << testPot.get_dimi() << " " << testPot.get_dimj() << " " << testPot.get_dimk() << std::endl;

    std::cout << "\% error (test vs ref): " << 100*std::abs(refPotSum-testPotSum)/refPotSum << std::endl;
    std::cout << "\% error (test vs ref2): " << 100*std::abs(refPotSum2-testPotSum)/refPotSum2 << std::endl;
    std::cout << "\% error (ref vs ref2): " << 100*std::abs(refPotSum-refPotSum2)/refPotSum2 << std::endl;
    BOOST_TEST(std::abs(refPotSum-testPotSum)/refPotSum<tol);
    BOOST_TEST(std::abs(refPotSum2-testPotSum)/refPotSum2<tol);

};


BOOST_AUTO_TEST_SUITE_END();

}