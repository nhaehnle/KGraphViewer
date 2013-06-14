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

#include "loadandlayoutjob.h"

#include <KDebug>
#include <KLocalizedString>

LoadAndLayoutJob::LoadAndLayoutJob(const QString& dotFileName, const QString& layoutCommand, QObject* parent) :
  Job(parent),
  m_dotFileName(dotFileName),
  m_layoutCommand(layoutCommand),
  m_g(0),
  m_gvc(0)
{
}

LoadAndLayoutJob::~LoadAndLayoutJob()
{
  if (m_gvc) {
    if (m_g) {
      gvFreeLayout(m_gvc, m_g);
      agclose(m_g);
    }
    gvFreeContext(m_gvc);
  }
}

void LoadAndLayoutJob::run()
{
  kDebug() << "LoadAndLayoutJob: " << m_dotFileName;

  m_gvc = gvContext();

  FILE* fp = fopen(m_dotFileName.toUtf8().data(), "r");
  if (!fp) {
    m_error = QString(i18n("Could not open temporary dotfile '%1' containing the graph")).arg(m_dotFileName);
    return;
  }
  m_g = agread(fp);
  fclose(fp);
  if (!m_g) {
    m_error = QString(i18n("Reading graph from temporary dotfile '%1' was not successful")).arg(m_dotFileName);
    return;
  }

  kDebug() << " now layout the graph";

  gvLayout(m_gvc, m_g, m_layoutCommand.toUtf8().data());
  gvRender(m_gvc, m_g, "xdot", NULL);

  kDebug() << " LoadAndLayoutJob done";
}
