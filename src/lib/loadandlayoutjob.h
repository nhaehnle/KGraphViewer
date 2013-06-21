/*
    This file is part of KGraphViewer.
    Copyright (C) 2010  Gael de Chalendar <kleag@free.fr>
    Copyright (C) 2013  Nicolai Hähnle <nhaehnle@gmail.com>

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of
    the License, or (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef LOADANDLAYOUTTHREAD_H
#define LOADANDLAYOUTTHREAD_H

#include <threadweaver/Job.h>

#include <graphviz/gvc.h>

/**
 * ThreadWeaver job that loads a graph from a given filename and layouts the graph
 * using the given command.
 *
 * Once the job is done, the graph can be retrieved, though it may be NULL, e.g.
 * if there was a loading error (a user-facing error message can be retrieved).
 *
 * Ownership of the graph remains with this class.
 */
class LoadAndLayoutJob : public ThreadWeaver::Job
{
public:
  LoadAndLayoutJob(const QString& dotFileName, const QString& layoutCommand, QObject* parent = 0);
  virtual ~LoadAndLayoutJob();

  graph_t* graph() const {return m_g;}
  GVC_t* gvc() const {return m_gvc;}
  const QString& error() const {return m_error;}

  const QString& dotFileName() const {return m_dotFileName;}
  const QString& layoutCommand() const {return m_layoutCommand;}

protected:
  virtual void run();

private:
  QString m_dotFileName;
  QString m_layoutCommand;
  QString m_error;
  graph_t *m_g;
  GVC_t *m_gvc;
};

#endif // LOADAGRAPHTHREAD_H
