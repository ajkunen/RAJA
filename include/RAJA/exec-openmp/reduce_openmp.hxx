/*!
 ******************************************************************************
 *
 * \file
 *
 * \brief   Header file containing RAJA reduction templates for OpenMP
 *          OpenMP execution.
 *
 *          These methods should work on any platform that supports OpenMP.
 *
 ******************************************************************************
 */

#ifndef RAJA_forall_omp_HXX
#define RAJA_forall_omp_HXX

#if defined(RAJA_USE_OPENMP)

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//
// Copyright (c) 2016, Lawrence Livermore National Security, LLC.
//
// Produced at the Lawrence Livermore National Laboratory
//
// LLNL-CODE-689114
//
// All rights reserved.
//
// This file is part of RAJA.
//
// For additional details, please also read raja/README-license.txt.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
// * Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the disclaimer below.
//
// * Redistributions in binary form must reproduce the above copyright notice,
//   this list of conditions and the disclaimer (as noted below) in the
//   documentation and/or other materials provided with the distribution.
//
// * Neither the name of the LLNS/LLNL nor the names of its contributors may
//   be used to endorse or promote products derived from this software without
//   specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL LAWRENCE LIVERMORE NATIONAL SECURITY,
// LLC, THE U.S. DEPARTMENT OF ENERGY OR CONTRIBUTORS BE LIABLE FOR ANY
// DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
// DAMAGES  (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
// OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
// STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
// IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.
//
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#include "RAJA/config.hxx"

#include "RAJA/int_datatypes.hxx"

#include "RAJA/reducers.hxx"

#include "RAJA/MemUtils_CPU.hxx"


#if defined(_OPENMP)
#include <omp.h>
#endif


namespace RAJA {


/*!
 ******************************************************************************
 *
 * \brief  Min reducer class template for use in OpenMP execution.
 *
 *         For usage example, see reducers.hxx.
 *
 ******************************************************************************
 */
template <typename T>
class ReduceMin<omp_reduce, T> 
{
public:
   //
   // Constructor takes default value (default ctor is disabled).
   //
   explicit ReduceMin(T init_val)
   {
      m_is_copy = false;

      m_reduced_val = init_val;

      m_myID = getCPUReductionId();

      m_blockdata = getCPUReductionMemBlock(m_myID);

      int nthreads = omp_get_max_threads();
#pragma omp parallel for schedule(static, 1)
      for ( int i = 0; i < nthreads; ++i ) {
         m_blockdata[i*s_block_offset] = init_val ;
      }
   }

   //
   // Copy ctor.
   //
   ReduceMin( const ReduceMin<omp_reduce, T>& other )
   {
      *this = other;
      m_is_copy = true;
   }

   //
   // Destructor.
   //
   ~ReduceMin<omp_reduce, T>() 
   {
      if (!m_is_copy) {
         releaseCPUReductionId(m_myID);
         // free any data owned by reduction object
      }
   }

   //
   // Operator to retrieve min value (before object is destroyed).
   //
   operator T()
   {
      int nthreads = omp_get_max_threads();
      for ( int i = 0; i < nthreads; ++i ) {
         m_reduced_val = 
            RAJA_MIN(m_reduced_val, 
                     static_cast<T>(m_blockdata[i*s_block_offset]));
      }

      return m_reduced_val;
   }

   //
   // Min function that sets min for current thread.
   //
   ReduceMin<omp_reduce, T> min(T val) const 
   {
      int tid = omp_get_thread_num();
      int idx = tid*s_block_offset;
      m_blockdata[idx] = 
         RAJA_MIN(static_cast<T>(m_blockdata[idx]), val);

      return *this ;
   }

private:
   //
   // Default ctor is declared private and not implemented.
   //
   ReduceMin<omp_reduce, T>();

   static const int s_block_offset = 
      COHERENCE_BLOCK_SIZE/sizeof(CPUReductionBlockDataType); 

   bool m_is_copy;
   int m_myID;

   T m_reduced_val;

   CPUReductionBlockDataType* m_blockdata;
} ;

/*!
 ******************************************************************************
 *
 * \brief  Min-loc reducer class template for use in OpenMP execution.
 *
 *         For usage example, see reducers.hxx.
 *
 ******************************************************************************
 */
template <typename T>
class ReduceMinLoc<omp_reduce, T> 
{
public:
   //
   // Constructor takes default value (default ctor is disabled).
   //
   explicit ReduceMinLoc(T init_val, Index_type init_loc)
   {
      m_is_copy = false;

      m_reduced_val = init_val;

      m_myID = getCPUReductionId();

      m_blockdata = getCPUReductionMemBlock(m_myID);
      m_idxdata = getCPUReductionLocBlock(m_myID);

      int nthreads = omp_get_max_threads();
#pragma omp parallel for schedule(static, 1)
      for ( int i = 0; i < nthreads; ++i ) {
         m_blockdata[i*s_block_offset] = init_val ;
         m_idxdata[i*s_idx_offset] = init_loc ;
      }
   }

   //
   // Copy ctor.
   //
   ReduceMinLoc( const ReduceMinLoc<omp_reduce, T>& other )
   {
      *this = other;
      m_is_copy = true;
   }

   //
   // Destructor.
   //
   ~ReduceMinLoc<omp_reduce, T>() 
   {
      if (!m_is_copy) {
         releaseCPUReductionId(m_myID);
         // free any data owned by reduction object
      }
   }

   //
   // Operator to retrieve min value (before object is destroyed).
   //
   operator T()
   {
      int nthreads = omp_get_max_threads();
      for ( int i = 0; i < nthreads; ++i ) {
         if ( static_cast<T>(m_blockdata[i*s_block_offset]) <= m_reduced_val ) {
            m_reduced_val = m_blockdata[i*s_block_offset];
            m_reduced_idx = m_idxdata[i*s_idx_offset];
         } 
      }

      return m_reduced_val;
   }

   //
   // Operator to retrieve index value of min (before object is destroyed).
   //
   Index_type getMinLoc()
   {
      int nthreads = omp_get_max_threads();
      for ( int i = 0; i < nthreads; ++i ) {
         if ( static_cast<T>(m_blockdata[i*s_block_offset]) <= m_reduced_val ) {
            m_reduced_val = m_blockdata[i*s_block_offset];
            m_reduced_idx = m_idxdata[i*s_idx_offset];
         }
      }

      return m_reduced_idx;
   }

   //
   // Min-loc function that sets min for current thread.
   //
   ReduceMinLoc<omp_reduce, T> minloc(T val, Index_type idx) const 
   {
      int tid = omp_get_thread_num();
      if ( val <= static_cast<T>(m_blockdata[tid*s_block_offset]) ) {
         m_blockdata[tid*s_block_offset] = val;
         m_idxdata[tid*s_idx_offset] = idx;
      }

      return *this ;
   }

private:
   //
   // Default ctor is declared private and not implemented.
   //
   ReduceMinLoc<omp_reduce, T>();

   static const int s_block_offset = 
      COHERENCE_BLOCK_SIZE/sizeof(CPUReductionBlockDataType); 
   static const int s_idx_offset = 
      COHERENCE_BLOCK_SIZE/sizeof(Index_type); 

   bool m_is_copy;
   int m_myID;

   T m_reduced_val;
   Index_type m_reduced_idx;

   CPUReductionBlockDataType* m_blockdata;
   Index_type* m_idxdata;
} ;

/*!
 ******************************************************************************
 *
 * \brief  Max reducer class template for use in OpenMP execution.
 *
 *         For usage example, see reducers.hxx.
 *
 ******************************************************************************
 */
template <typename T>
class ReduceMax<omp_reduce, T> 
{
public:
   //
   // Constructor takes default value (default ctor is disabled).
   //
   explicit ReduceMax(T init_val)
   {
      m_is_copy = false;

      m_reduced_val = init_val;

      m_myID = getCPUReductionId();

      m_blockdata = getCPUReductionMemBlock(m_myID);

      int nthreads = omp_get_max_threads();
#pragma omp parallel for schedule(static, 1)
      for ( int i = 0; i < nthreads; ++i ) {
         m_blockdata[i*s_block_offset] = init_val ;
      }
   }

   //
   // Copy ctor.
   //
   ReduceMax( const ReduceMax<omp_reduce, T>& other )
   {
      *this = other;
      m_is_copy = true;
   }

   //
   // Destructor.
   //
   ~ReduceMax<omp_reduce, T>() 
   {
      if (!m_is_copy) {
         releaseCPUReductionId(m_myID);
         // free any data owned by reduction object
      }
   }

   //
   // Operator to retrieve max value (before object is destroyed).
   //
   operator T()
   {
      int nthreads = omp_get_max_threads();
      for ( int i = 0; i < nthreads; ++i ) {
         m_reduced_val = 
            RAJA_MAX(m_reduced_val, 
                     static_cast<T>(m_blockdata[i*s_block_offset]));
      }

      return m_reduced_val;
   }

   //
   // Max function that sets max for current thread.
   //
   ReduceMax<omp_reduce, T> max(T val) const 
   {
      int tid = omp_get_thread_num();
      int idx = tid*s_block_offset;
      m_blockdata[idx] = 
         RAJA_MAX(static_cast<T>(m_blockdata[idx]), val);

      return *this ;
   }

private:
   //
   // Default ctor is declared private and not implemented.
   //
   ReduceMax<omp_reduce, T>();

   static const int s_block_offset = 
      COHERENCE_BLOCK_SIZE/sizeof(CPUReductionBlockDataType); 

   bool m_is_copy;
   int m_myID;

   T m_reduced_val;

   CPUReductionBlockDataType* m_blockdata;
} ;

/*!
 ******************************************************************************
 *
 * \brief  Max-loc reducer class template for use in OpenMP execution.
 *
 *         For usage example, see reducers.hxx.
 *
 ******************************************************************************
 */
template <typename T>
class ReduceMaxLoc<omp_reduce, T> 
{
public:
   //
   // Constructor takes default value (default ctor is disabled).
   //
   explicit ReduceMaxLoc(T init_val, Index_type init_loc)
   {
      m_is_copy = false;

      m_reduced_val = init_val;

      m_myID = getCPUReductionId();

      m_blockdata = getCPUReductionMemBlock(m_myID);
      m_idxdata = getCPUReductionLocBlock(m_myID);

      int nthreads = omp_get_max_threads();
#pragma omp parallel for schedule(static, 1)
      for ( int i = 0; i < nthreads; ++i ) {
         m_blockdata[i*s_block_offset] = init_val ;
         m_idxdata[i*s_idx_offset] = init_loc ;
      }
   }

   //
   // Copy ctor.
   //
   ReduceMaxLoc( const ReduceMinLoc<omp_reduce, T>& other )
   {
      *this = other;
      m_is_copy = true;
   }

   //
   // Destructor.
   //
   ~ReduceMaxLoc<omp_reduce, T>() 
   {
      if (!m_is_copy) {
         releaseCPUReductionId(m_myID);
         // free any data owned by reduction object
      }
   }

   //
   // Operator to retrieve max value (before object is destroyed).
   //
   operator T()
   {
      int nthreads = omp_get_max_threads();
      for ( int i = 0; i < nthreads; ++i ) {
         if ( static_cast<T>(m_blockdata[i*s_block_offset]) >= m_reduced_val ) {
            m_reduced_val = m_blockdata[i*s_block_offset];
            m_reduced_idx = m_idxdata[i*s_idx_offset];
         } 
      }

      return m_reduced_val;
   }

   //
   // Operator to retrieve index value of max (before object is destroyed).
   //
   Index_type getMaxLoc()
   {
      int nthreads = omp_get_max_threads();
      for ( int i = 0; i < nthreads; ++i ) {
         if ( static_cast<T>(m_blockdata[i*s_block_offset]) >= m_reduced_val ) {
            m_reduced_val = m_blockdata[i*s_block_offset];
            m_reduced_idx = m_idxdata[i*s_idx_offset];
         }
      }

      return m_reduced_idx;
   }

   //
   // Max-loc function that sets max for current thread.
   //
   ReduceMaxLoc<omp_reduce, T> maxloc(T val, Index_type idx) const 
   {
      int tid = omp_get_thread_num();
      if ( val >= static_cast<T>(m_blockdata[tid*s_block_offset]) ) {
         m_blockdata[tid*s_block_offset] = val;
         m_idxdata[tid*s_idx_offset] = idx;
      }

      return *this ;
   }

private:
   //
   // Default ctor is declared private and not implemented.
   //
   ReduceMaxLoc<omp_reduce, T>();

   static const int s_block_offset = 
      COHERENCE_BLOCK_SIZE/sizeof(CPUReductionBlockDataType); 
   static const int s_idx_offset = 
      COHERENCE_BLOCK_SIZE/sizeof(Index_type); 

   bool m_is_copy;
   int m_myID;

   T m_reduced_val;
   Index_type m_reduced_idx;

   CPUReductionBlockDataType* m_blockdata;
   Index_type* m_idxdata;
} ;

/*!
 ******************************************************************************
 *
 * \brief  Sum reducer class template for use in OpenMP execution.
 *
 *         For usage example, see reducers.hxx.
 *
 ******************************************************************************
 */
template <typename T>
class ReduceSum<omp_reduce, T> 
{
public:
   //
   // Constructor takes default value (default ctor is disabled).
   //
   explicit ReduceSum(T init_val)
   {
      m_is_copy = false;

      m_init_val = init_val;
      m_reduced_val = static_cast<T>(0);

      m_myID = getCPUReductionId();

      m_blockdata = getCPUReductionMemBlock(m_myID);

      int nthreads = omp_get_max_threads();
#pragma omp parallel for schedule(static, 1)
      for ( int i = 0; i < nthreads; ++i ) {
         m_blockdata[i*s_block_offset] = 0 ;
      }
   }

   //
   // Copy ctor.
   //
   ReduceSum( const ReduceSum<omp_reduce, T>& other )
   {
      *this = other;
      m_is_copy = true;
   }

   //
   // Destructor.
   //
   ~ReduceSum<omp_reduce, T>() 
   {
      if (!m_is_copy) {
         releaseCPUReductionId(m_myID);
         // free any data owned by reduction object
      }
   }

   //
   // Operator to retrieve sum value (before object is destroyed).
   //
   operator T()
   {
      T tmp_reduced_val = static_cast<T>(0);
      int nthreads = omp_get_max_threads();
      for ( int i = 0; i < nthreads; ++i ) {
         tmp_reduced_val += static_cast<T>(m_blockdata[i*s_block_offset]);
      }
      m_reduced_val = m_init_val + tmp_reduced_val;

      return m_reduced_val;
   }

   //
   // += operator that performs accumulation for current thread.
   //
   ReduceSum<omp_reduce, T> operator+=(T val) const 
   {
      int tid = omp_get_thread_num();
      m_blockdata[tid*s_block_offset] += val;
      return *this ;
   }

private:
   //
   // Default ctor is declared private and not implemented.
   //
   ReduceSum<omp_reduce, T>();

   static const int s_block_offset = 
      COHERENCE_BLOCK_SIZE/sizeof(CPUReductionBlockDataType); 

   bool m_is_copy;
   int m_myID;

   T m_init_val;
   T m_reduced_val;

   CPUReductionBlockDataType* m_blockdata;
} ;


}  // closing brace for RAJA namespace

#endif  // closing endif for RAJA_USE_CUDA guard

#endif  // closing endif for header file include guard
