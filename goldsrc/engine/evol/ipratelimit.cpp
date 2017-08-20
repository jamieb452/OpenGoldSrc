/*
*
*    This program is free software; you can redistribute it and/or modify it
*    under the terms of the GNU General Public License as published by the
*    Free Software Foundation; either version 2 of the License, or (at
*    your option) any later version.
*
*    This program is distributed in the hope that it will be useful, but
*    WITHOUT ANY WARRANTY; without even the implied warranty of
*    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
*    General Public License for more details.
*
*    You should have received a copy of the GNU General Public License
*    along with this program; if not, write to the Free Software Foundation,
*    Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*
*    In addition, as a special exception, the author gives permission to
*    link the code of this program with the Half-Life Game Engine ("HL
*    Engine") and Modified Game Libraries ("MODs") developed by Valve,
*    L.L.C ("Valve").  You must obey the GNU General Public License in all
*    respects for all of the code used other than the HL Engine and MODs
*    from Valve.  If you modify this file, you may extend this exception
*    to your version of the file, but you are not obligated to do so.  If
*    you do not wish to do so, delete this exception statement from your
*    version.
*
*/

//#include "precompiled.h"
#include <ctime>
#include "quakedef.h"
#include "ipratelimit.h"

bool CIPRateLimit::LessIP( const iprate_t& lhs, const iprate_t& rhs )
{
	return lhs.ip < rhs.ip;
}

CIPRateLimit::CIPRateLimit()
	: m_IPTree( 0, START_TREE_SIZE, &CIPRateLimit::LessIP )
{
}

bool CIPRateLimit::CheckIP( netadr_t adr )
{
	const time_t curTime = time( nullptr );

	if( m_IPTree.Count() > MAX_TREE_SIZE )
	{
		//Remove timed out IPs until we're below the threshold.
		for( auto i = m_IPTree.FirstInorder(); m_IPTree.Count() >= static_cast<unsigned int>( floor( MAX_TREE_SIZE / 1.5 ) ) && i != m_IPTree.InvalidIndex(); )
		{
			auto& ip = m_IPTree[ i ];

			if( ( curTime - ip.lastTime ) > FLUSH_TIMEOUT && ip.ip != *reinterpret_cast<int*>( adr.ip ) )
			{
				auto next = m_IPTree.NextInorder( i );
				m_IPTree.RemoveAt( i );
				i = next;
			}
			else
			{
				i = m_IPTree.NextInorder( i );
			}
		}
	}

	iprate_t findEntry;
	findEntry.count = 0;
	findEntry.lastTime = 0;
	findEntry.ip = *reinterpret_cast<int*>( adr.ip );

	auto i = m_IPTree.Find( findEntry );

	if( i == m_IPTree.InvalidIndex() )
	{
		iprate_t newEntry;
		newEntry.count = 1;
		newEntry.lastTime = curTime;
		newEntry.ip = *reinterpret_cast<int*>( adr.ip );

		m_IPTree.Insert( newEntry );
	}
	else
	{
		auto& ip = m_IPTree[ i ];

		++ip.count;

		if( ( curTime - ip.lastTime ) > max_queries_window.value )
		{
			ip.lastTime = curTime;
			ip.count = 1;
		}
		else
		{
			if( ip.count / max_queries_window.value > max_queries_sec.value )
				return false;
		}
	}

	++m_iGlobalCount;

	if( ( curTime - m_lLastTime ) <= max_queries_window.value )
	{
		return m_iGlobalCount / max_queries_window.value <= max_queries_sec_global.value;
	}
	else
	{
		m_iGlobalCount = 1;
		m_lLastTime = curTime;
		return true;
	}
}
