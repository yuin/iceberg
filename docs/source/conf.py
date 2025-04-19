#vim: fileencoding=utf8

import sys, os
from datetime import date

PROJECT_DIR = os.path.join(os.path.dirname(__file__), '..', '..')
sys.path.append(os.path.dirname(__file__))
sys.path.append(PROJECT_DIR)

release = [
  line for line in open(os.path.join(PROJECT_DIR, "src", "ib_constants.h"))
  if "IB_VERSION" in line
][0].split('"')[-2]
project = "iceberg"
author = "Yusuke Inuzuka"
copyright = '2012-%s, %s' % (str(date.today().year), author)

# Extension
extensions = ["sphinxcontrib.luadomain"]

# Source
master_doc = 'index'
templates_path = ['_templates']
source_suffix = '.rst'
exclude_trees = []
pygments_style = 'sphinx'

# html build settings
html_theme = 'haiku'
html_logo = "iceberg.png"

# htmlhelp settings
htmlhelp_basename = '%sdoc' % project

# latex build settings
latex_documents = [
  ('index', '%s.tex' % project, u'%s Documentation' % project, author,
   'manual'),
]
