#include "../include/MaterialProperties.h"
#include <cassert>
#include <iostream>

void testMaterialProperties() {
    MaterialProperties props;
    Stack stack = props.getStack(1);
    assert(stack.layers.size() == 4 && "Stack should have 4 layers");
    assert(stack.layers[0].material.name == "TPS" && "First layer should be TPS");
    assert(std::abs(props.getCarbonFiberThickness(0.5) -  0.0001160660171779821) < 1e-3 && "Carbon fiber thickness incorrect");
    assert(props.getMinTPSThickness() == 0.0001 && "Min TPS thickness incorrect");
    assert(props.getMaxTPSThickness() == 0.01 && "Max TPS thickness incorrect");
    std::cout << "MaterialProperties test passed.\n";
}

int main() {
    testMaterialProperties();
    return 0;
}