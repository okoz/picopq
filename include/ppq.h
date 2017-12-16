#pragma once

#include <stdexcept>
#include <cstdint>

namespace ppq
{
	namespace
	{
		#include <libpq-fe.h>

		constexpr uint16_t byte_swap_16( uint16_t value )
		{
			return ( value & 0xFF ) << 8 | ( value >> 8 );
		}

		constexpr uint32_t byte_swap_32( uint32_t value )
		{
			return ( byte_swap_16( value & 0xFFFF ) << 16 )
				| byte_swap_16( value >> 16 );
		}

		constexpr uint64_t byte_swap_64( uint64_t value )
		{
			return ( static_cast< uint64_t >( byte_swap_32( value & 0xFFFFFFFF ) ) << 32 )
				| byte_swap_32( value >> 32 );
		}

		template< typename T >
		char const* const_charify( T const& value ) { return reinterpret_cast< char const* >( &value ); }

		template<>
		char const* const_charify( char const* const& value ) { return value; }

		template< typename T >
		constexpr int param_length( T const& value ) { return sizeof( T ); }

		template<>
		constexpr int param_length< char const* >( char const* const& value ) { return 0; }

		// Convert values into network format.

		template< typename T >
		constexpr T to_network( T value ) { static_assert( !std::is_same_v< T, T >, "Unknown type" ); }

		template<>
		constexpr uint8_t to_network( uint8_t value ) { return value; }

		template<>
		constexpr uint16_t to_network( uint16_t value ) { return byte_swap_16( value ); }

		template<>
		constexpr uint32_t to_network( uint32_t value ) { return byte_swap_32( value ); }

		template<>
		constexpr uint64_t to_network( uint64_t value ) { return byte_swap_64( value ); }

		template<>
		constexpr int8_t to_network( int8_t value ) { return value; }

		template<>
		constexpr int16_t to_network( int16_t value ) { return byte_swap_16( value ); }

		template<>
		constexpr int32_t to_network( int32_t value ) { return byte_swap_32( value ); }

		template<>
		constexpr int64_t to_network( int64_t value ) { return byte_swap_64( value ); }

		template< typename T, typename U >
		union conversion_helper
		{
			static_assert( sizeof( T ) == sizeof( U ) );

			T a;
			U b;
		};

		template<>
		constexpr float to_network( float value )
		{
			conversion_helper< float, uint32_t > helper{};
			helper.a = value;
			helper.b = byte_swap_32( helper.b );
			return helper.a;
		}

		template<>
		constexpr double to_network( double value )
		{
			conversion_helper< double, uint64_t > helper{};
			helper.a = value;
			helper.b = byte_swap_64( helper.b );
			return helper.a;
		}

		// Pointers get sent straight through.

		template< typename T >
		constexpr T* to_network( T* value ) { return value; }

		// Somewhat more complicated support for arrays and vectors.

		template< typename T >
		constexpr Oid get_oid() { static_assert( !std::is_same_v< T, T >, "Unknown type" ); }

		template<>
		constexpr Oid get_oid< int64_t >() { return 20; }

		template< typename T >
		struct vector_helper
		{
#pragma pack( push, 4 )
			struct pq_array
			{
				int32_t n_dimensions;
				int32_t data_offset;
				Oid element_type;

				int32_t size;
				int32_t index;

				// Followed by data.

				pq_array( Oid element_type, size_t size )
					: n_dimensions( byte_swap_32( 1 ) )
					, data_offset{ 0 }
					, element_type( byte_swap_32( element_type ) )
					, size( byte_swap_32( static_cast< int32_t >( size ) ) )
					, index{ 0 }
				{
				}
			};
#pragma pack( pop )

#pragma pack( push, 1 )
			struct pq_value
			{
				int32_t length;
				T value;
			};
#pragma pack( pop )

		public:
			vector_helper( std::vector< T > const& v )
				: bytes_( sizeof( pq_array ) + v.size() * sizeof( pq_value ) )
			{
				constexpr int32_t value_length = byte_swap_32( sizeof( T ) );

				pq_array* header = new ( bytes_.data() ) pq_array( get_oid< T >(), v.size() );
				pq_value* element = reinterpret_cast< pq_value* >( header + 1 );

				for ( T const& value : v )
				{
					element->length = value_length;
					element->value = to_network( value );
					element++;
				}
			}

			vector_helper( vector_helper const& ) = delete;
			vector_helper( vector_helper&& ) = default;

			char const* data() const { return bytes_.data(); }
			int length() const { return static_cast< int >( bytes_.size() ); }

		private:
			std::vector< char > bytes_;
		};

		template< typename T >
		vector_helper< T > to_network( std::vector< T > const& value ) { return vector_helper< T >( value ); }

		template< typename T >
		char const* const_charify( vector_helper< T > const& value ) { return value.data(); }

		template< typename T >
		int param_length( vector_helper< T > const& value ) { return value.length(); }

		// Convert values from network format.

		template< typename T >
		constexpr T from_network( char const* value ) { return T( value ); }

		template<>
		constexpr uint8_t from_network( char const* value ) { return *reinterpret_cast< uint8_t const* >( value ); }

		template<>
		constexpr uint16_t from_network( char const* value ) { return byte_swap_16( *reinterpret_cast< uint16_t const* >( value ) ); }

		template<>
		constexpr uint32_t from_network( char const* value ) { return byte_swap_32( *reinterpret_cast< uint32_t const* >( value ) ); }

		template<>
		constexpr uint64_t from_network( char const* value ) { return byte_swap_64( *reinterpret_cast< uint64_t const* >( value ) ); }

		template<>
		constexpr int8_t from_network( char const* value ) { return *reinterpret_cast< int8_t const* >( value ); }

		template<>
		constexpr int16_t from_network( char const* value ) { return byte_swap_16( *reinterpret_cast< int16_t const* >( value ) ); }

		template<>
		constexpr int32_t from_network( char const* value ) { return byte_swap_32( *reinterpret_cast< int32_t const* >( value ) ); }

		template<>
		constexpr int64_t from_network( char const* value ) { return byte_swap_64( *reinterpret_cast< int64_t const* >( value ) ); }

		template<>
		constexpr float from_network( char const* value )
		{
			conversion_helper< float, uint32_t > helper{};
			helper.b = byte_swap_32( *reinterpret_cast< uint32_t const* >( value ) );
			return helper.a;
		}

		template<>
		constexpr double from_network( char const* value )
		{
			conversion_helper< double, uint64_t > helper{};
			helper.b = byte_swap_64( *reinterpret_cast< uint64_t const* >( value ) );
			return helper.a;
		}
	}

	class exception : public std::runtime_error
	{
	public:
		using std::runtime_error::runtime_error;
	};

	class row
	{
	public:
		row( row const& ) = default;
		row( row&& ) = default;

		template< typename T >
		T at( size_t i )
		{
			char* value = PQgetvalue( result_, static_cast< int >( row_ ), static_cast< int >( i ) );
			return from_network< T >( value );
		}

	protected:
		friend class result;

		row( size_t r, PGresult* res )
			: row_{ r }
			, result_{ res }
		{
		}

		row& operator=( const row& ) = default;

	private:
		size_t row_{};
		PGresult* result_{};
	};

	class result
	{
	public:
		result( result const& ) = delete;
		result( result&& r )
			: result_{ r.result_ }
		{
			r.result_ = nullptr;
		}

		~result()
		{
			if ( result_ )
				PQclear( result_ );
		}

		size_t size() const { return PQntuples( result_ ); }

		row at( size_t i ) const { return row( i, result_ ); }

		class iterator : public std::iterator< std::input_iterator_tag, row >
		{
		public:
			iterator() = delete;
			iterator( iterator const& ) = default;
			iterator& operator=( iterator const& ) = default;

			void swap( iterator& i )
			{
				std::swap( i_, i.i_ );
				std::swap( result_, i.result_ );
			}

			iterator& operator++()
			{
				i_++;
				row_ = row{ i_, result_ };
				return *this;
			}

			iterator operator++( int )
			{
				iterator copy( *this );
				return ++copy;
			}

			value_type const& operator*() const { return row_; }
			value_type& operator*() { return row_; }

			value_type const* operator->() const { return &row_; }
			value_type* operator->() { return &row_; }

			bool operator==( iterator const& i ) const { return i_ == i.i_ && result_ == i.result_; }
			bool operator!=( iterator const& i ) const { return !( *this == i ); }

		protected:
			friend class result;

			iterator( size_t i, PGresult* result )
				: row_{ i, result }
				, i_ { i }
				, result_{ result }
			{
			}

		private:
			row row_;
			size_t i_{};
			PGresult* result_{};
		};

		iterator begin() { return iterator( 0, result_ ); }
		iterator end() { return iterator( size(), result_ ); }

	protected:
		friend class connection;

		result( PGresult* res )
			: result_{ res }
		{

		}

		void check()
		{
			ExecStatusType status = PQresultStatus( result_ );
			if ( !( status == PGRES_COMMAND_OK || status == PGRES_TUPLES_OK ) )
				throw exception( PQresultErrorMessage( result_ ) );
		}

	private:
		PGresult* result_{};
	};

	namespace
	{
		template< typename T >
		constexpr int is_binary = 1;

		template<>
		constexpr int is_binary< char const* > = 0;
	}

	class connection
	{
	public:
		connection( char const* connection_string )
		{
			connection_ = PQconnectdb( connection_string );
			if ( PQstatus( connection_ ) != CONNECTION_OK )
				throw exception( "Couldn't connect to database" );

		}

		connection( connection const& ) = delete;
		connection( connection&& c )
			: connection_{ c.connection_ }
		{
			c.connection_ = nullptr;
		}

		~connection()
		{
			if ( connection_ )
				PQfinish( connection_ );
		}

		void prepare( char const* name, char const* query )
		{
			result result( PQprepare( connection_, name, query, 0, nullptr ) );
			result.check();
		}

		template< typename... Ts >
		result execute( char const* query, Ts const&... args )
		{
			return really_execute( PQexecParams, std::make_index_sequence< sizeof...( Ts ) >{}, query, args... );
		}

		template< typename... Ts >
		result execute_prepared( char const* name, Ts const&... args )
		{
			return really_execute( PQexecPreparedWrapper, std::make_index_sequence< sizeof...( Ts ) >{}, name, args... );
		}

	protected:
		static PGresult *PQexecPreparedWrapper(
			PGconn *conn,
			const char *command,
			int nParams,
			const Oid *paramTypes,
			const char *const *paramValues,
			const int *paramLengths,
			const int *paramFormats,
			int resultFormat )
		{
			return PQexecPrepared(
				conn,
				command,
				nParams,
				paramValues,
				paramLengths,
				paramFormats,
				resultFormat
				);
		}

		template< typename E, typename... Ts, size_t... Is >
		result really_execute( E executor, std::index_sequence< Is... >, const char* query, Ts const&... args )
		{
			constexpr int num_params = sizeof...( args );

			if constexpr( num_params == 0 )
			{
				result res( executor(
					connection_,
					query,
					0,
					nullptr,
					nullptr,
					nullptr,
					nullptr,
					1
				) );

				res.check();

				return res;
			}
			else
			{
				auto params = std::make_tuple( to_network( args )... );
				char const* param_values[] = { const_charify( std::get< Is >( params ) )... };
				int const param_lengths[] = { param_length( std::get< Is >( params ) )... };
				int const param_formats[] = { is_binary< Ts >... };

				result res( executor(
					connection_,
					query,
					num_params,
					nullptr,
					param_values,
					param_lengths,
					param_formats,
					1
				) );

				res.check();

				return res;
			}
		}

	private:
		PGconn* connection_{};
	};
}