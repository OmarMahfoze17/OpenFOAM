/*****************************************************************************
 * Multiscale Universal Interface Code Coupling Library                       *
 *                                                                            *
 * Copyright (C) 2019 Y. H. Tang, S. Kudo, X. Bian, Z. Li, G. E. Karniadakis  *
 *                                                                            *
 * This software is jointly licensed under the Apache License, Version 2.0    *
 * and the GNU General Public License version 3, you may use it according     *
 * to either.                                                                 *
 *                                                                            *
 * ** Apache License, version 2.0 **                                          *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License");            *
 * you may not use this file except in compliance with the License.           *
 * You may obtain a copy of the License at                                    *
 *                                                                            *
 * http://www.apache.org/licenses/LICENSE-2.0                                 *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 *                                                                            *
 * ** GNU General Public License, version 3 **                                *
 *                                                                            *
 * This program is free software: you can redistribute it and/or modify       *
 * it under the terms of the GNU General Public License as published by       *
 * the Free Software Foundation, either version 3 of the License, or          *
 * (at your option) any later version.                                        *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.      *
 *****************************************************************************/

#ifndef DUMMYOF_CONFIG_H
#define DUMMYOF_CONFIG_H

#include "/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/MUI/src/general/util.h"
#include "/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/MUI/src/general/dim.h"
#include "/media/omar/bb5bfc6d-a551-4b3e-9f47-2b7d74410f28/WORK/codes/MUI/src/general/exception.h"

namespace mui {

	struct mui_config {

		using EXCEPTION = exception_segv;

		static const bool DEBUG = false;
		static const int D = 3;
		static const bool FIXEDPOINTS = false;
		static const bool QUIET = false;


		using REAL = double;
		using INT  = int64_t;
		using time_type = REAL;
		using iterator_type = INT;
		using point_type = point<REAL, D>;
		using data_types = type_list<uint32_t, 
		      							 uint64_t,
		      							 int32_t,
									 int64_t,
									 double,
									 double_t,
									 float,
									 std::string
									>;

	};
}

#endif
