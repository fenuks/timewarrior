////////////////////////////////////////////////////////////////////////////////
//
// Copyright 2016 - 2021, Thomas Lauf, Paul Beckingham, Federico Hernandez.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
// https://www.opensource.org/licenses/mit-license.php
//
////////////////////////////////////////////////////////////////////////////////

#include <cmake.h>
#include <Datetime.h>
#include <format.h>
#include <commands.h>
#include <timew.h>
#include <iostream>
#include <stdlib.h>
#include <IntervalFilterAllWithIds.h>

////////////////////////////////////////////////////////////////////////////////
int CmdMove (
  const CLI& cli,
  Rules& rules,
  Database& database,
  Journal& journal)
{
  const bool verbose = rules.getBoolean ("verbose");

  // Gather ID and TAGs.
  std::set <int> ids = cli.getIds ();

  if (ids.size() > 1)
  {
    throw std::string("The 'move' command only supports a single ID.");
  }

  if (ids.empty())
  {
    throw std::string ("ID must be specified. See 'timew help move'.");
  }

  journal.startTransaction ();

  int id = *ids.begin ();

  std::string new_start;
  for (auto& arg : cli._args)
  {
    if (arg.hasTag ("FILTER") && arg._lextype == Lexer::Type::date)
    {
      new_start = arg.attribute ("raw");
    }
  }

  auto filtering = IntervalFilterAllWithIds (ids);
  auto intervals = getTracked (database, rules, filtering);

  if (intervals.size () != ids.size ())
  {
    for (auto& id: ids)
    {
      bool found = false;

      for (auto& interval: intervals)
      {
        if (interval.id == id)
        {
          found = true;
          break;
        }
      }
      if (!found)
      {
        throw format ("ID '@{1}' does not correspond to any tracking.", id);
      }
    }
  }

  Interval interval = intervals.at (0);

  if (interval.synthetic)
  {
    flattenDatabase (database, rules);
    filtering.reset ();
    intervals = getTracked (database, rules, filtering);
    interval = intervals.at (0);
  }

  // Move start time.
  Datetime start (new_start);

  // Changing the start date should also change the end date by the same
  // amount.
  if (interval.start < start)
  {
    auto delta = start - interval.start;
    interval.start = start;
    if (! interval.is_open ())
    {
      interval.end += delta;
    }
  }
  else
  {
    auto delta = interval.start - start;
    interval.start = start;
    if (! interval.is_open ())
    {
      interval.end -= delta;
    }
  }

  database.deleteInterval (intervals.at (0));

  validate (cli, rules, database, interval);
  database.addInterval (interval, verbose);

  journal.endTransaction ();

  if (verbose)
  {
    std::cout << "Moved @" << id << " to " << interval.start.toISOLocalExtended () << '\n';
  }

  return 0;
}

////////////////////////////////////////////////////////////////////////////////
