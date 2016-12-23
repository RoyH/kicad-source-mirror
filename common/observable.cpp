/*
* This program source code file is part of KICAD, a free EDA CAD application.
*
* Copyright (C) 2016 Kicad Developers, see change_log.txt for contributors.
*
* Graphics Abstraction Layer (GAL) for OpenGL
*
* Shader class
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License
* as published by the Free Software Foundation; either version 2
* of the License, or (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, you may find one here:
* http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
* or you may search the http://www.gnu.org website for the version 2 license,
* or you may write to the Free Software Foundation, Inc.,
* 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
*/

#include "observable.h"
#include <algorithm>

namespace UTIL {

    namespace DETAIL {

        template< typename T >
        struct equals {
            equals( const T& val ) : val_( val ) {}

            bool operator()( const T& val ) {
                return val == val_;
            }

        private:
            const T& val_;
        };

        OBSERVABLE_BASE::IMPL::IMPL( OBSERVABLE_BASE* owned_by )
            : owned_by_( owned_by ), iteration_count_( 0 )
        {}

        bool OBSERVABLE_BASE::IMPL::is_shared() const
        {
            return owned_by_ == nullptr;
        }

        void OBSERVABLE_BASE::IMPL::set_shared()
        {
            owned_by_ = nullptr;
        }

        OBSERVABLE_BASE::IMPL::~IMPL()
        {
        }

        void OBSERVABLE_BASE::IMPL::remove_observer( void* observer )
        {
            if(iteration_count_) {
                for(auto*& ptr : observers_) {
                    if(ptr == observer) {
                        ptr = nullptr;
                    }
                }
            }
            else {
                collect();
                if(observers_.empty() && owned_by_) {
                    owned_by_->on_observers_empty();
                }
            }
        }

        void OBSERVABLE_BASE::IMPL::collect()
        {
            auto it = std::remove_if( observers_.begin(), observers_.end(), DETAIL::equals<void*>( nullptr ) );
            observers_.erase( it, observers_.end() );
        }
    }

    LINK::LINK()
    {
    }

    LINK::LINK( std::shared_ptr<DETAIL::OBSERVABLE_BASE::IMPL> token, void* observer )
        : token_( std::move( token ) ), observer_( observer )
    {
    }

    LINK::LINK( LINK&& other )
        : token_( std::move( other.token_ ) ), observer_( other.observer_ )
    {
        other.token_.reset();
    }

    LINK& LINK::operator=( LINK&& other )
    {
        token_ = std::move( other.token_ );
        other.token_.reset();
        observer_ = other.observer_;
        return *this;
    }

    LINK::operator bool() const {
        return token_ ? true : false;
    }

    LINK::~LINK()
    {
        reset();
    }

    void LINK::reset()
    {
        if(token_) {
            token_->remove_observer( observer_ );
        }
    }

    namespace DETAIL {

        OBSERVABLE_BASE::OBSERVABLE_BASE()
        {
        }

        OBSERVABLE_BASE::OBSERVABLE_BASE( OBSERVABLE_BASE& other )
            : impl_( other.get_shared_impl() )
        {
        }

        OBSERVABLE_BASE::~OBSERVABLE_BASE()
        {
        }

        size_t OBSERVABLE_BASE::size() const {
            if(impl_) {
                return impl_->observers_.size();
            }
            else {
                return 0;
            }
        }

        void OBSERVABLE_BASE::remove_observer( void* observer ) {
            assert( impl_ );
            impl_->remove_observer( observer );
        }

        void OBSERVABLE_BASE::allocate_impl() {
            if(!impl_) {
                impl_ = std::make_shared<IMPL>( this );
            }
        }

        void OBSERVABLE_BASE::allocate_shared_impl()
        {
            if(!impl_) {
                impl_ = std::make_shared<IMPL>();
            }
            else {
                impl_->set_shared();
            }
        }

        void OBSERVABLE_BASE::deallocate_impl() {
            impl_.reset();
        }

        void OBSERVABLE_BASE::enter_iteration() {
            if(impl_) {
                ++impl_->iteration_count_;
            }
        }

        void OBSERVABLE_BASE::leave_iteration() {
            if(impl_) {
                --impl_->iteration_count_;
                if(impl_->iteration_count_ == 0) {
                    if(!impl_->is_shared() && impl_.use_count() == 1) {
                        impl_.reset();
                    }
                    else {
                        impl_->collect();
                    }
                }
            }
        }

        void OBSERVABLE_BASE::on_observers_empty()
        {
            // called by an impl that is owned by this, ie. it is a non-shared impl
            // also it is not iterating
            deallocate_impl();
        }

        std::shared_ptr<OBSERVABLE_BASE::IMPL> OBSERVABLE_BASE::get_shared_impl()
        {
            allocate_shared_impl();
            return impl_;
        }

        void OBSERVABLE_BASE::add_observer( void* observer ) {
            allocate_impl();
            impl_->observers_.push_back( observer );
        }

    }

}
