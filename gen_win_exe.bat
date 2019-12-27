pyinstaller viewpca.in ^
--onefile ^
--hidden-import=numpy.random.common ^
--hidden-import=numpy.random.bounded_integers ^
--hidden-import=numpy.random.common ^
--hidden-import=numpy.random.entropy ^
--hidden-import=PySide2.QtXml ^
--hidden-import=sklearn.utils._cython_blas ^
--add-data="ViewPCA/ui;ViewPCA/ui"