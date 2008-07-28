/*BHEADER**********************************************************************
 * Copyright (c) 2008,  Lawrence Livermore National Security, LLC.
 * Produced at the Lawrence Livermore National Laboratory.
 * This file is part of HYPRE.  See file COPYRIGHT for details.
 *
 * HYPRE is free software; you can redistribute it and/or modify it under the
 * terms of the GNU Lesser General Public License (as published by the Free
 * Software Foundation) version 2.1 dated February 1999.
 *
 * $Revision$
 ***********************************************************************EHEADER*/

/*
   This test driver performs the following operations:

1. Read a linear system corresponding to a parallel finite element
   discretization of Maxwell's equations.

2. Call the AMS solver in HYPRE to solve that linear system.
*/

/* hypre/AMS prototypes */
#include "_hypre_parcsr_ls.h"

void CheckIfFileExists(char *file)
{
   FILE *test;
   if (!(test = fopen(file,"r")))
   {
      MPI_Finalize();
      printf("Can't find the input file \"%s\"\n",file);
      exit(1);
   }
   fclose(test);
}

int main (int argc, char *argv[])
{
   int num_procs, myid;
   int time_index;

   int solver_id;
   int maxit, cycle_type, rlx_type, rlx_sweeps, dim;
   double rlx_weight, rlx_omega;
   int amg_coarsen_type, amg_rlx_type, amg_agg_levels, amg_agg_npaths, amg_interp_type, amg_Pmax;
   int h1_method, singular_problem, coordinates;
   double tol, theta;
   int blockSize;
   HYPRE_Solver solver, precond;

   HYPRE_ParCSRMatrix A, G, Aalpha=0, Abeta=0, M=0;
   HYPRE_ParVector x0, b;
   HYPRE_ParVector Gx=0, Gy=0, Gz=0;
   HYPRE_ParVector x=0, y=0, z=0;

   /* Initialize MPI */
   MPI_Init(&argc, &argv);
   MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
   MPI_Comm_rank(MPI_COMM_WORLD, &myid);

   /* Set defaults */
   solver_id = 3;
   maxit = 100;
   tol = 1e-6;
   dim = 3;
   coordinates = 0;
   h1_method = 0;
   singular_problem = 0;
   rlx_type = 2; rlx_sweeps = 1;
   rlx_weight = 1.0; rlx_omega = 1.0;
   cycle_type = 1; amg_coarsen_type = 10; amg_agg_levels = 1; amg_rlx_type = 6;       /* HMIS-1 */
   /* cycle_type = 1; amg_coarsen_type = 10; amg_agg_levels = 0; amg_rlx_type = 3; */ /* HMIS-0 */
   /* cycle_type = 1; amg_coarsen_type = 8; amg_agg_levels = 1; amg_rlx_type = 3;  */ /* PMIS-1 */
   /* cycle_type = 1; amg_coarsen_type = 8; amg_agg_levels = 0; amg_rlx_type = 3;  */ /* PMIS-0 */
   /* cycle_type = 7; amg_coarsen_type = 6; amg_agg_levels = 0; amg_rlx_type = 6;  */ /* Falgout-0 */
   amg_interp_type = 6; amg_Pmax = 4; amg_agg_npaths = 1;        /* long-range interpolation */
   /* amg_interp_type = 0; amg_Pmax = 0; amg_agg_npaths = 1; */  /* standard interpolation */
   theta = 0.25;
   blockSize = 5;

   /* Parse command line */
   {
      int arg_index = 0;
      int print_usage = 0;

      while (arg_index < argc)
      {
         if ( strcmp(argv[arg_index], "-solver") == 0 )
         {
            arg_index++;
            solver_id = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-maxit") == 0 )
         {
            arg_index++;
            maxit = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-tol") == 0 )
         {
            arg_index++;
            tol = atof(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-type") == 0 )
         {
            arg_index++;
            cycle_type = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-rlx") == 0 )
         {
            arg_index++;
            rlx_type = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-rlxn") == 0 )
         {
            arg_index++;
            rlx_sweeps = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-rlxw") == 0 )
         {
            arg_index++;
            rlx_weight = atof(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-rlxo") == 0 )
         {
            arg_index++;
            rlx_omega = atof(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-ctype") == 0 )
         {
            arg_index++;
            amg_coarsen_type = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-amgrlx") == 0 )
         {
            arg_index++;
            amg_rlx_type = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-agg") == 0 )
         {
            arg_index++;
            amg_agg_levels = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-aggnp") == 0 )
         {
            arg_index++;
            amg_agg_npaths = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-itype") == 0 )
         {
            arg_index++;
            amg_interp_type = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-pmax") == 0 )
         {
            arg_index++;
            amg_Pmax = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-dim") == 0 )
         {
            arg_index++;
            dim = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-coord") == 0 )
         {
            arg_index++;
            coordinates = 1;
         }
         else if ( strcmp(argv[arg_index], "-h1") == 0 )
         {
            arg_index++;
            h1_method = 1;
         }
         else if ( strcmp(argv[arg_index], "-sing") == 0 )
         {
            arg_index++;
            singular_problem = 1;
         }
         else if ( strcmp(argv[arg_index], "-theta") == 0 )
         {
            arg_index++;
            theta = atof(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-bsize") == 0 )
         {
            arg_index++;
            blockSize = atoi(argv[arg_index++]);
         }
         else if ( strcmp(argv[arg_index], "-help") == 0 )
         {
            print_usage = 1;
            break;
         }
         else
         {
            arg_index++;
         }
      }

      if (argc == 1)
         print_usage = 1;

      if ((print_usage) && (myid == 0))
      {
         printf("\n");
         printf("Usage: mpirun -np <np> %s [<options>]\n", argv[0]);
         printf("\n");
         printf("  Hypre solvers options:                                       \n");
         printf("    -solver <ID>         : solver ID                           \n");
         printf("                           0  - AMG                            \n");
         printf("                           1  - AMG-PCG                        \n");
         printf("                           2  - AMS                            \n");
         printf("                           3  - AMS-PCG (default)              \n");
         printf("                           4  - DS-PCG                         \n");
         printf("                           5  - AME eigensolver                \n");
         printf("    -maxit <num>         : maximum number of iterations (100)  \n");
         printf("    -tol <num>           : convergence tolerance (1e-6)        \n");
         printf("\n");
         printf("  AMS solver options:                                          \n");
         printf("    -dim <num>           : space dimension                     \n");
         printf("    -type <num>          : 3-level cycle type (0-8, 11-14)     \n");
         printf("    -theta <num>         : BoomerAMG threshold (0.25)          \n");
         printf("    -ctype <num>         : BoomerAMG coarsening type           \n");
         printf("    -agg <num>           : Levels of BoomerAMG agg. coarsening \n");
         printf("    -aggnp <num>         : Number of paths in agg. coarsening  \n");
         printf("    -amgrlx <num>        : BoomerAMG relaxation type           \n");
         printf("    -itype <num>         : BoomerAMG interpolation type        \n");
         printf("    -pmax <num>          : BoomerAMG interpolation truncation  \n");
         printf("    -rlx <num>           : relaxation type                     \n");
         printf("    -rlxn <num>          : number of relaxation sweeps         \n");
         printf("    -rlxw <num>          : damping parameter (usually <=1)     \n");
         printf("    -rlxo <num>          : SOR parameter (usuallyin (0,2))     \n");
         printf("    -coord               : use coordinate vectors              \n");
         printf("    -h1                  : use block-diag Poisson solves       \n");
         printf("    -sing                : curl-curl only (singular) problem   \n");
         printf("\n");
         printf("  AME eigensolver options:                                     \n");
         printf("    -bsize<num>          : number of eigenvalues to compute    \n");
         printf("\n");
      }

      if (print_usage)
      {
         MPI_Finalize();
         return (0);
      }
   }

   CheckIfFileExists("aFEM.A.D.0");
   CheckIfFileExists("aFEM.x0.0");
   CheckIfFileExists("aFEM.b.0");
   CheckIfFileExists("aFEM.G.D.0");

   HYPRE_ParCSRMatrixRead(MPI_COMM_WORLD, "aFEM.A", &A);
   HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.x0", &x0);
   HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.b", &b);
   HYPRE_ParCSRMatrixRead(MPI_COMM_WORLD, "aFEM.G", &G);

   /* Vectors Gx, Gy and Gz */
   if (!coordinates)
   {
      CheckIfFileExists("aFEM.Gx.0");
      CheckIfFileExists("aFEM.Gy.0");
      CheckIfFileExists("aFEM.Gz.0");
      HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.Gx", &Gx);
      HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.Gy", &Gy);
      if (dim == 3)
         HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.Gz", &Gz);
   }

   /* Vectors x, y and z */
   if (coordinates)
   {
      CheckIfFileExists("aFEM.x.0");
      CheckIfFileExists("aFEM.y.0");
      CheckIfFileExists("aFEM.z.0");
      HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.x", &x);
      HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.y", &y);
      if (dim == 3)
         HYPRE_ParVectorRead(MPI_COMM_WORLD, "aFEM.z", &z);
   }

   /* Poisson matrices */
   if (h1_method)
   {
      CheckIfFileExists("aFEM.Aalpha.D.0");
      HYPRE_ParCSRMatrixRead(MPI_COMM_WORLD, "aFEM.Aalpha", &Aalpha);
      CheckIfFileExists("aFEM.Abeta.D.0");
      HYPRE_ParCSRMatrixRead(MPI_COMM_WORLD, "aFEM.Abeta", &Abeta);
   }

   if (!myid)
      printf("Problem size: %d\n\n",
             hypre_ParCSRMatrixGlobalNumRows((hypre_ParCSRMatrix*)A));

   MPI_Barrier(MPI_COMM_WORLD);

   /* AMG */
   if (solver_id == 0)
   {
      int num_iterations;
      double final_res_norm;

      /* Start timing */
      time_index = hypre_InitializeTiming("BoomerAMG Setup");
      hypre_BeginTiming(time_index);

      /* Create solver */
      HYPRE_BoomerAMGCreate(&solver);

      /* Set some parameters (See Reference Manual for more parameters) */
      HYPRE_BoomerAMGSetPrintLevel(solver, 3);  /* print solve info + parameters */
      HYPRE_BoomerAMGSetCoarsenType(solver, 6); /* Falgout coarsening */
      HYPRE_BoomerAMGSetRelaxType(solver, rlx_type); /* G-S/Jacobi hybrid relaxation */
      HYPRE_BoomerAMGSetNumSweeps(solver, 1);   /* Sweeeps on each level */
      HYPRE_BoomerAMGSetMaxLevels(solver, 20);  /* maximum number of levels */
      HYPRE_BoomerAMGSetTol(solver, tol);       /* conv. tolerance */
      HYPRE_BoomerAMGSetMaxIter(solver, maxit); /* maximum number of iterations */
      HYPRE_BoomerAMGSetStrongThreshold(solver, theta);

      HYPRE_BoomerAMGSetup(solver, A, b, x0);

      /* Finalize setup timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Setup phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      /* Start timing again */
      time_index = hypre_InitializeTiming("BoomerAMG Solve");
      hypre_BeginTiming(time_index);

      /* Solve */
      HYPRE_BoomerAMGSolve(solver, A, b, x0);

      /* Finalize solve timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Solve phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      /* Run info - needed logging turned on */
      HYPRE_BoomerAMGGetNumIterations(solver, &num_iterations);
      HYPRE_BoomerAMGGetFinalRelativeResidualNorm(solver, &final_res_norm);
      if (myid == 0)
      {
         printf("\n");
         printf("Iterations = %d\n", num_iterations);
         printf("Final Relative Residual Norm = %e\n", final_res_norm);
         printf("\n");
      }

      /* Destroy solver */
      HYPRE_BoomerAMGDestroy(solver);
   }

   /* AMS */
   if (solver_id == 2)
   {
      /* Start timing */
      time_index = hypre_InitializeTiming("AMS Setup");
      hypre_BeginTiming(time_index);

      /* Create solver */
      HYPRE_AMSCreate(&solver);

      /* Set AMS parameters */
      HYPRE_AMSSetDimension(solver, dim);
      HYPRE_AMSSetMaxIter(solver, maxit);
      HYPRE_AMSSetTol(solver, tol);
      HYPRE_AMSSetCycleType(solver, cycle_type);
      HYPRE_AMSSetPrintLevel(solver, 1);
      HYPRE_AMSSetDiscreteGradient(solver, G);

      /* Vectors Gx, Gy and Gz */
      if (!coordinates)
         HYPRE_AMSSetEdgeConstantVectors(solver,Gx,Gy,Gz);

      /* Vectors x, y and z */
      if (coordinates)
         HYPRE_AMSSetCoordinateVectors(solver,x,y,z);

      /* Poisson matrices */
      if (h1_method)
      {
         HYPRE_AMSSetAlphaPoissonMatrix(solver, Aalpha);
         HYPRE_AMSSetBetaPoissonMatrix(solver, Abeta);
      }

      if (singular_problem)
         HYPRE_AMSSetBetaPoissonMatrix(solver, NULL);

      /* Smoothing and AMG options */
      HYPRE_AMSSetSmoothingOptions(solver, rlx_type, rlx_sweeps, rlx_weight, rlx_omega);
      HYPRE_AMSSetAlphaAMGOptions(solver, amg_coarsen_type, amg_agg_levels, amg_rlx_type, theta, amg_interp_type, amg_Pmax);
      HYPRE_AMSSetBetaAMGOptions(solver, amg_coarsen_type, amg_agg_levels, amg_rlx_type, theta, amg_interp_type, amg_Pmax);

      HYPRE_AMSSetup(solver, A, b, x0);

      /* Finalize setup timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Setup phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      /* Start timing again */
      time_index = hypre_InitializeTiming("AMS Solve");
      hypre_BeginTiming(time_index);

      /* Solve */
      HYPRE_AMSSolve(solver, A, b, x0);

      /* Finalize solve timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Solve phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      /* Destroy solver */
      HYPRE_AMSDestroy(solver);
   }

   /* PCG solvers */
   else if (solver_id == 1 || solver_id == 3 || solver_id == 4)
   {
      int num_iterations;
      double final_res_norm;

      /* Start timing */
      if (solver_id == 1)
         time_index = hypre_InitializeTiming("BoomerAMG-PCG Setup");
      else if (solver_id == 3)
         time_index = hypre_InitializeTiming("AMS-PCG Setup");
      else if (solver_id == 4)
         time_index = hypre_InitializeTiming("DS-PCG Setup");
      hypre_BeginTiming(time_index);

      /* Create solver */
      HYPRE_ParCSRPCGCreate(MPI_COMM_WORLD, &solver);

      /* Set some parameters (See Reference Manual for more parameters) */
      HYPRE_PCGSetMaxIter(solver, maxit); /* max iterations */
      HYPRE_PCGSetTol(solver, tol); /* conv. tolerance */
      HYPRE_PCGSetTwoNorm(solver, 0); /* use the two norm as the stopping criteria */
      HYPRE_PCGSetPrintLevel(solver, 2); /* print solve info */
      HYPRE_PCGSetLogging(solver, 1); /* needed to get run info later */

      /* PCG with AMG preconditioner */
      if (solver_id == 1)
      {
         /* Now set up the AMG preconditioner and specify any parameters */
         HYPRE_BoomerAMGCreate(&precond);
         HYPRE_BoomerAMGSetPrintLevel(precond, 1);  /* print amg solution info */
         HYPRE_BoomerAMGSetCoarsenType(precond, 6); /* Falgout coarsening */
         HYPRE_BoomerAMGSetRelaxType(precond, rlx_type);   /* Sym G.S./Jacobi hybrid */
         HYPRE_BoomerAMGSetNumSweeps(precond, 1);   /* Sweeeps on each level */
         HYPRE_BoomerAMGSetMaxLevels(precond, 20);  /* maximum number of levels */
         HYPRE_BoomerAMGSetTol(precond, 0.0);      /* conv. tolerance (if needed) */
         HYPRE_BoomerAMGSetMaxIter(precond, 1);     /* do only one iteration! */
         HYPRE_BoomerAMGSetStrongThreshold(precond, theta);

         /* Set the PCG preconditioner */
         HYPRE_PCGSetPrecond(solver,
                             (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSolve,
                             (HYPRE_PtrToSolverFcn) HYPRE_BoomerAMGSetup,
                             precond);
      }
      /* PCG with AMS preconditioner */
      if (solver_id == 3)
      {
         /* Now set up the AMS preconditioner and specify any parameters */
         HYPRE_AMSCreate(&precond);
         HYPRE_AMSSetDimension(precond, dim);
         HYPRE_AMSSetMaxIter(precond, 1);
         HYPRE_AMSSetTol(precond, 0.0);
         HYPRE_AMSSetCycleType(precond, cycle_type);
         HYPRE_AMSSetPrintLevel(precond, 0);
         HYPRE_AMSSetDiscreteGradient(precond, G);

         /* Vectors Gx, Gy and Gz */
         if (!coordinates)
            HYPRE_AMSSetEdgeConstantVectors(precond,Gx,Gy,Gz);

         /* Vectors x, y and z */
         if (coordinates)
            HYPRE_AMSSetCoordinateVectors(precond,x,y,z);

         /* Poisson matrices */
         if (h1_method)
         {
            HYPRE_AMSSetAlphaPoissonMatrix(precond, Aalpha);
            HYPRE_AMSSetBetaPoissonMatrix(precond, Abeta);
         }

         if (singular_problem)
            HYPRE_AMSSetBetaPoissonMatrix(precond, NULL);

         /* Smoothing and AMG options */
         HYPRE_AMSSetSmoothingOptions(precond, rlx_type, rlx_sweeps, rlx_weight, rlx_omega);
         HYPRE_AMSSetAlphaAMGOptions(precond, amg_coarsen_type, amg_agg_levels, amg_rlx_type, theta, amg_interp_type, amg_Pmax);
         HYPRE_AMSSetBetaAMGOptions(precond, amg_coarsen_type, amg_agg_levels, amg_rlx_type, theta, amg_interp_type, amg_Pmax);

         /* Set the PCG preconditioner */
         HYPRE_PCGSetPrecond(solver,
                             (HYPRE_PtrToSolverFcn) HYPRE_AMSSolve,
                             (HYPRE_PtrToSolverFcn) HYPRE_AMSSetup,
                             precond);
      }
      /* PCG with diagonal scaling preconditioner */
      else if (solver_id == 4)
      {
         /* Set the PCG preconditioner */
         HYPRE_PCGSetPrecond(solver,
                             (HYPRE_PtrToSolverFcn) HYPRE_ParCSRDiagScale,
                             (HYPRE_PtrToSolverFcn) HYPRE_ParCSRDiagScaleSetup,
                             NULL);
      }

      /* Setup */
      HYPRE_ParCSRPCGSetup(solver, A, b, x0);

      /* Finalize setup timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Setup phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      /* Start timing again */
      if (solver_id == 1)
         time_index = hypre_InitializeTiming("BoomerAMG-PCG Solve");
      else if (solver_id == 3)
         time_index = hypre_InitializeTiming("AMS-PCG Solve");
      else if (solver_id == 4)
         time_index = hypre_InitializeTiming("DS-PCG Solve");
      hypre_BeginTiming(time_index);

      /* Solve */
      HYPRE_ParCSRPCGSolve(solver, A, b, x0);

      /* Finalize solve timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Solve phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      /* Run info - needed logging turned on */
      HYPRE_PCGGetNumIterations(solver, &num_iterations);
      HYPRE_PCGGetFinalRelativeResidualNorm(solver, &final_res_norm);
      if (myid == 0)
      {
         printf("\n");
         printf("Iterations = %d\n", num_iterations);
         printf("Final Relative Residual Norm = %e\n", final_res_norm);
         printf("\n");
      }

      /* Destroy solver and preconditioner */
      HYPRE_ParCSRPCGDestroy(solver);
      if (solver_id == 1)
         HYPRE_BoomerAMGDestroy(precond);
      else if (solver_id == 3)
         HYPRE_AMSDestroy(precond);
   }

   if (solver_id == 5)
   {
      CheckIfFileExists("aFEM.M.D.0");
      HYPRE_ParCSRMatrixRead(MPI_COMM_WORLD, "aFEM.M", &M);

      time_index = hypre_InitializeTiming("AME Setup");
      hypre_BeginTiming(time_index);

      /* Create AMS preconditioner and specify any parameters */
      HYPRE_AMSCreate(&precond);
      HYPRE_AMSSetDimension(precond, dim);
      HYPRE_AMSSetMaxIter(precond, 1);
      HYPRE_AMSSetTol(precond, 0.0);
      HYPRE_AMSSetCycleType(precond, cycle_type);
      HYPRE_AMSSetPrintLevel(precond, 0);
      HYPRE_AMSSetDiscreteGradient(precond, G);

      /* Vectors Gx, Gy and Gz */
      if (!coordinates)
         HYPRE_AMSSetEdgeConstantVectors(precond,Gx,Gy,Gz);

      /* Vectors x, y and z */
      if (coordinates)
         HYPRE_AMSSetCoordinateVectors(precond,x,y,z);

      /* Poisson matrices */
      if (h1_method)
      {
         HYPRE_AMSSetAlphaPoissonMatrix(precond, Aalpha);
         HYPRE_AMSSetBetaPoissonMatrix(precond, Abeta);
      }

      if (singular_problem)
         HYPRE_AMSSetBetaPoissonMatrix(precond, NULL);

      /* Set up the AMS preconditioner */
      HYPRE_AMSSetup(precond, A, b, x0);

      /* Smoothing and AMG options */
      HYPRE_AMSSetSmoothingOptions(precond, rlx_type, rlx_sweeps, rlx_weight, rlx_omega);
      HYPRE_AMSSetAlphaAMGOptions(precond, amg_coarsen_type, amg_agg_levels, amg_rlx_type, theta, amg_interp_type, amg_Pmax);
      HYPRE_AMSSetBetaAMGOptions(precond, amg_coarsen_type, amg_agg_levels, amg_rlx_type, theta, amg_interp_type, amg_Pmax);

      /* Create AME object */
      HYPRE_AMECreate(&solver);

      /* Set main parameters */
      HYPRE_AMESetAMSSolver(solver, precond);
      HYPRE_AMESetMassMatrix(solver, M);
      HYPRE_AMESetBlockSize(solver, blockSize);

      /* Set additional parameters */
      HYPRE_AMESetMaxIter(solver, maxit); /* max iterations */
      HYPRE_AMESetTol(solver, tol); /* conv. tolerance */
      if (myid == 0)
         HYPRE_AMESetPrintLevel(solver, 1); /* print solve info */
      else
         HYPRE_AMESetPrintLevel(solver, 0);

      /* Setup */
      HYPRE_AMESetup(solver);

      /* Finalize setup timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Setup phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      time_index = hypre_InitializeTiming("AME Solve");
      hypre_BeginTiming(time_index);

      /* Solve */
      HYPRE_AMESolve(solver);

      /* Finalize solve timing */
      hypre_EndTiming(time_index);
      hypre_PrintTiming("Solve phase times", MPI_COMM_WORLD);
      hypre_FinalizeTiming(time_index);
      hypre_ClearTiming();

      /* Destroy solver and preconditioner */
      HYPRE_AMEDestroy(solver);
      HYPRE_AMSDestroy(precond);
   }

   /* Save the solution */
   /* HYPRE_ParVectorPrint(x0,"x.ams"); */

   /* Clean-up */
   HYPRE_ParCSRMatrixDestroy(A);
   HYPRE_ParVectorDestroy(x0);
   HYPRE_ParVectorDestroy(b);
   HYPRE_ParCSRMatrixDestroy(G);

   if (M) HYPRE_ParCSRMatrixDestroy(M);

   if (Gx) HYPRE_ParVectorDestroy(Gx);
   if (Gy) HYPRE_ParVectorDestroy(Gy);
   if (Gz) HYPRE_ParVectorDestroy(Gz);

   if (x) HYPRE_ParVectorDestroy(x);
   if (y) HYPRE_ParVectorDestroy(y);
   if (z) HYPRE_ParVectorDestroy(z);

   if (Aalpha) HYPRE_ParCSRMatrixDestroy(Aalpha);
   if (Abeta)  HYPRE_ParCSRMatrixDestroy(Abeta);

   MPI_Finalize();

   if (HYPRE_GetError() && !myid)
      fprintf(stderr,"hypre_error_flag = %d\n", HYPRE_GetError());

   return 0;
}
