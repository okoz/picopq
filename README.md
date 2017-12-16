# PicoPQ

Tiny C++17 wrapper for libpq providing a binary interface to PostgreSQL.

## Dependencies

- libpq

## Usage

The table created using:

```SQL
CREATE TABLE characters( id bigint, name text );
```

can be queried with:

```C++
#include <ppq.h>

try
{
    ppq::connection connection( "postgresql://user:password@server/database" );

    ppq::result result = connection.execute( "SELECT id, name FROM characters WHERE id = $1", 1ll );
    for ( ppq::row row : result )
        std::cout << row.at< int64_t >( 0 ) << " - " << row.at< char const* >( 1 ) << std::endl;
}
catch ( ppq::exception const& e )
{
    std::cout << e.what() << std::endl;
}
```

There is also a way to prepare queries:

```C++
connection.prepare( "get_character", "SELECT id, name FROM characters WHERE id = $1" );
connection.execute_prepared( "get_characters", 1ll );
```

Parameters of type std::vector are also supported:

can be queried with:

```C++
std::vector< int64_t > ids{ 1ll, 2ll };
ppq::result result = connection.execute( "SELECT name FROM characters WHERE id = ANY($1)", ids );
```

## Extensions
Other types can be added by providing a specialization of

```C++
template< typename T >
constexpr T to_network( T value );
```

which should return the data in a format that can be sent over the wire and

```C++
template< typename T >
constexpr T from_network( char const* value );
```

which will construct an object from the network data.  By default it tries to call the constructor `T( char const* )`.