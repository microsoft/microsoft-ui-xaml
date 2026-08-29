THIS DIRECTORY SHOULD ONLY BE USED FOR AUTHORING INTEGRATION TESTS AGAINST TOOLS EXTERNAL TO JUPITER

PLEASE AVOID AUTHORING NEW TESTS UNDER THIS FOLDER HIERARCHY FOR ANY OTHER PURPOSE.

For true unit tests please author them in xcp\components after refactoring your code to build in isolation.

For C++/WRL tests against Jupiter please consider authoring the test in C++/CX. 
WRL::ComPtr is available in this environment and should allow you to accomplish your goals.

For tests against external tools you may continue authoring them in this folder, it will eventually
be trimmed out and be left for only this purpose (with a possible rename).