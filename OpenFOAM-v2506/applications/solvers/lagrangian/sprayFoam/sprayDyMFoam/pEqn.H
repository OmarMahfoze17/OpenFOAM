rho = thermo.rho();
rho.clamp_range(rhoMin, rhoMax);
rho.relax();

volScalarField rAU(1.0/UEqn.A());
surfaceScalarField rhorAUf("rhorAUf", fvc::interpolate(rho*rAU));
volVectorField HbyA(constrainHbyA(rAU*UEqn.H(), U, p));

if (pimple.nCorrPISO() <= 1)
{
    tUEqn.clear();
}

if (pimple.transonic())
{
    surfaceScalarField phid
    (
        "phid",
        fvc::interpolate(psi)
       *(
            fvc::flux(HbyA)
          + MRF.zeroFilter
            (
                rhorAUf*fvc::ddtCorr(rho, U, rhoUf)/fvc::interpolate(rho)
            )
        )
    );

    fvc::makeRelative(phid, psi, U);
    MRF.makeRelative(fvc::interpolate(psi), phid);

    while (pimple.correctNonOrthogonal())
    {
        fvScalarMatrix pEqn
        (
            fvm::ddt(psi, p)
          + fvm::div(phid, p)
          - fvm::laplacian(rhorAUf, p)
         ==
            parcels.Srho()
          + fvOptions(psi, p, rho.name())
        );

        pEqn.solve(p.select(pimple.finalInnerIter()));

        if (pimple.finalNonOrthogonalIter())
        {
            phi == pEqn.flux();
        }
    }
}
else
{
    surfaceScalarField phiHbyA
    (
        "phiHbyA",
        fvc::flux(rho*HbyA)
      + MRF.zeroFilter(rhorAUf*fvc::ddtCorr(rho, U, rhoUf))
    );

    fvc::makeRelative(phiHbyA, rho, U);
    MRF.makeRelative(fvc::interpolate(rho), phiHbyA);

    // Update the pressure BCs to ensure flux consistency
    constrainPressure(p, rho, U, phiHbyA, rhorAUf, MRF);

    while (pimple.correctNonOrthogonal())
    {
        fvScalarMatrix pEqn
        (
            fvm::ddt(psi, p)
          + fvc::div(phiHbyA)
          - fvm::laplacian(rhorAUf, p)
         ==
            parcels.Srho()
          + fvOptions(psi, p, rho.name())
        );

        pEqn.solve(p.select(pimple.finalInnerIter()));

        if (pimple.finalNonOrthogonalIter())
        {
            phi = phiHbyA + pEqn.flux();
        }
    }
}

#include "rhoEqn.H"
#include "compressibleContinuityErrs.H"

// Explicitly relax pressure for momentum corrector
p.relax();

// Recalculate density from the relaxed pressure
rho = thermo.rho();
rho.clamp_range(rhoMin, rhoMax);
rho.relax();
Info<< "rho min/max : " << min(rho).value() << " " << max(rho).value() << endl;

U = HbyA - rAU*fvc::grad(p);
U.correctBoundaryConditions();
fvOptions.correct(U);
K = 0.5*magSqr(U);

{
    rhoUf = fvc::interpolate(rho*U);
    surfaceVectorField n(mesh.Sf()/mesh.magSf());
    rhoUf += n*(fvc::absolute(phi, rho, U)/mesh.magSf() - (n & rhoUf));
}

if (thermo.dpdt())
{
    dpdt = fvc::ddt(p);

    if (mesh.moving())
    {
        dpdt -= fvc::div(fvc::meshPhi(rho, U), p);
    }
}
