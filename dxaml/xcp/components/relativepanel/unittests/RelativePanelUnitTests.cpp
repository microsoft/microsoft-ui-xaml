// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License. See LICENSE in the project root for license information.

#include "precomp.h"

#include "RelativePanelUnitTests.h"

#include "RPNode.h"
#include "RPGraph.h"
#include "CDependencyObject.h"
#include "UIElement.h"
#include "LayoutStorage.h"

namespace
{
    RPNode* AddNodeToGraph(
        _In_ RPGraph* graph, 
        _In_ CUIElement* element, 
        _In_ float width, 
        _In_ float height)
    {    
        element->EnsureLayoutStorage();
        element->GetLayoutStorage()->m_desiredSize.width = width;
        element->GetLayoutStorage()->m_desiredSize.height = height;

        RPNode node(element);
        graph->GetNodes().push_front(std::move(node));

        return &graph->GetNodes().front();
    }
}


namespace Windows { namespace UI { namespace Xaml { namespace Tests { namespace Controls {

void RelativePanelUnitTests::VerifyHorizontalDependencyResolution()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 300, 100);

    node0->SetAlignHorizontalCenterWithConstraint(node1);
    node1->SetAlignTopWithPanelConstraint(true);

    {
        const XRECTF finalRect{ 0, 0, 300, 100 };
        const XRECTF mr0{ 0, 0, 300, 100 };
        const XRECTF mr1{ 0, 0, 300, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 300, 100 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }
}

void RelativePanelUnitTests::VerifyVerticalDependencyResolution()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 300);

    node0->SetAlignVerticalCenterWithConstraint(node1);
    node1->SetAlignTopWithPanelConstraint(true);

    {
        const XRECTF finalRect{ 0, 0, 100, 300 };
        const XRECTF mr0{ 0, 0, 100, 300 };
        const XRECTF mr1{ 0, 0, 100, 300 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 300 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }
}

void RelativePanelUnitTests::VerifyDownwardPrecedence()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1, e2;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 50);

    node0->SetAlignTopWithPanelConstraint(true);
    node1->SetBelowConstraint(node0);
    node2->SetAlignVerticalCenterWithPanelConstraint(true);
    node2->SetBelowConstraint(node1);
    node2->SetAlignVerticalCenterWithConstraint(node1);
    node2->SetAlignTopWithConstraint(node1);
    node2->SetAlignTopWithPanelConstraint(true);

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 100, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 200 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 0, 100, 50 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 100, 100, 100 };
        const XRECTF mr2{ 0, 100, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 100, 100, 50 };

        node2->SetAlignTopWithPanelConstraint(false);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 100, 100, 100 };
        const XRECTF mr2{ 0, 100, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 125, 100, 50 };

        node2->SetAlignTopWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 250 };
        const XRECTF mr0{ 0, 0, 100, 250 };
        const XRECTF mr1{ 0, 100, 100, 150 };
        const XRECTF mr2{ 0, 200, 100, 50 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 200, 100, 50 };

        node2->SetAlignVerticalCenterWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 100, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 200 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 75, 100, 50 };

        node2->SetBelowConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifyUpwardPrecedence()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1, e2;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 50);

    node0->SetAlignBottomWithPanelConstraint(true);
    node1->SetAboveConstraint(node0);
    node2->SetAlignVerticalCenterWithPanelConstraint(true);
    node2->SetAboveConstraint(node1);
    node2->SetAlignVerticalCenterWithConstraint(node1);
    node2->SetAlignBottomWithConstraint(node1);
    node2->SetAlignBottomWithPanelConstraint(true);

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 200 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 0, 150, 100, 50 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 100 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 0, 50, 100, 50 };

        node2->SetAlignBottomWithPanelConstraint(false);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 100 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 0, 25, 100, 50 };

        node2->SetAlignBottomWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 250 };
        const XRECTF mr0{ 0, 0, 100, 250 };
        const XRECTF mr1{ 0, 0, 100, 150 };
        const XRECTF mr2{ 0, 0, 100, 50 };
        const XRECTF ar0{ 0, 150, 100, 100 };
        const XRECTF ar1{ 0, 50, 100, 100 };
        const XRECTF ar2{ 0, 0, 100, 50 };

        node2->SetAlignVerticalCenterWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 200 };
        const XRECTF mr0{ 0, 0, 100, 200 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 200 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 0, 75, 100, 50 };

        node2->SetAboveConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifyRightwardPrecedence()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1, e2;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 50, 100);

    node0->SetAlignLeftWithPanelConstraint(true);
    node1->SetRightOfConstraint(node0);
    node2->SetAlignHorizontalCenterWithPanelConstraint(true);
    node2->SetRightOfConstraint(node1);
    node2->SetAlignHorizontalCenterWithConstraint(node1);
    node2->SetAlignLeftWithConstraint(node1);
    node2->SetAlignLeftWithPanelConstraint(true);

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 100, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 200, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 0, 0, 50, 100 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 100, 0, 100, 100 };
        const XRECTF mr2{ 100, 0, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 100, 0, 50, 100 };

        node2->SetAlignLeftWithPanelConstraint(false);
        
        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 100, 0, 100, 100 };
        const XRECTF mr2{ 100, 0, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 125, 0, 50, 100 };

        node2->SetAlignLeftWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 250, 100 };
        const XRECTF mr0{ 0, 0, 250, 100 };
        const XRECTF mr1{ 100, 0, 150, 100 };
        const XRECTF mr2{ 200, 0, 50, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 200, 0, 50, 100 };

        node2->SetAlignHorizontalCenterWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 100, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 200, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 75, 0, 50, 100 };

        node2->SetRightOfConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifyLeftwardPrecedence()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1, e2;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 50, 100);

    node0->SetAlignRightWithPanelConstraint(true);
    node1->SetLeftOfConstraint(node0);
    node2->SetAlignHorizontalCenterWithPanelConstraint(true);
    node2->SetLeftOfConstraint(node1);
    node2->SetAlignHorizontalCenterWithConstraint(node1);
    node2->SetAlignRightWithConstraint(node1);
    node2->SetAlignRightWithPanelConstraint(true);

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 200, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 150, 0, 50, 100 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 50, 0, 50, 100 };

        node2->SetAlignRightWithPanelConstraint(false);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 100, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 25, 0, 50, 100 };

        node2->SetAlignRightWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 250, 100 };
        const XRECTF mr0{ 0, 0, 250, 100 };
        const XRECTF mr1{ 0, 0, 150, 100 };
        const XRECTF mr2{ 0, 0, 50, 100 };
        const XRECTF ar0{ 150, 0, 100, 100 };
        const XRECTF ar1{ 50, 0, 100, 100 };
        const XRECTF ar2{ 0, 0, 50, 100 };

        node2->SetAlignHorizontalCenterWithConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 100 };
        const XRECTF mr0{ 0, 0, 200, 100 };
        const XRECTF mr1{ 0, 0, 100, 100 };
        const XRECTF mr2{ 0, 0, 200, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 75, 0, 50, 100 };

        node2->SetLeftOfConstraint(nullptr);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifyVerticallyFloatingAndOverlapping()
{
    const XSIZEF availableSize{ 2000.0f, 2000.0f };
    const XRECTF finalRect{ 0, 0, 100, 200 };
    const XRECTF mr0{ 0, 0, 100, 200 };
    const XRECTF mr1{ 0, 0, 100, 200 };
    const XRECTF mr2{ 0, 0, 100, 200 };
    const XRECTF mr3{ 0, 0, 100, 200 };
    const XRECTF ar0{ 0, 0, 100, 200 };
    const XRECTF ar1{ 0, 0, 100, 100 };
    const XRECTF ar2{ 0, 100, 100, 100 };
    const XRECTF ar3{ 0, 50, 100, 100 };

    RPGraph graph;
    CUIElement e0, e1, e2, e3;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100.0f, 200.0f);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100.0f, 100.0f);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 100.0f, 100.0f);
    RPNode* node3 = AddNodeToGraph(&graph, &e3, 100.0f, 100.0f);

    node1->SetAlignTopWithPanelConstraint(true);
    node2->SetAlignBottomWithPanelConstraint(true);
    node3->SetAlignVerticalCenterWithPanelConstraint(true);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
    panelSize = graph.CalculateDesiredSize();
    VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

    VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
    VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
    VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
    VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
    VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
    VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
    VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
    VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
}

void RelativePanelUnitTests::VerifyHorizontallyFloatingAndOverlapping()
{
    const XSIZEF availableSize{ 2000.0f, 2000.0f };
    const XRECTF finalRect{ 0, 0, 200, 100 };
    const XRECTF mr0{ 0, 0, 200, 100 };
    const XRECTF mr1{ 0, 0, 200, 100 };
    const XRECTF mr2{ 0, 0, 200, 100 };
    const XRECTF mr3{ 0, 0, 200, 100 };
    const XRECTF ar0{ 0, 0, 200, 100 };
    const XRECTF ar1{ 0, 0, 100, 100 };
    const XRECTF ar2{ 100, 0, 100, 100 };
    const XRECTF ar3{ 50, 0, 100, 100 };

    RPGraph graph;
    CUIElement e0, e1, e2, e3;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 200.0f, 100.0f);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100.0f, 100.0f);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 100.0f, 100.0f);
    RPNode* node3 = AddNodeToGraph(&graph, &e3, 100.0f, 100.0f);

    node1->SetAlignLeftWithPanelConstraint(true);
    node2->SetAlignRightWithPanelConstraint(true);
    node3->SetAlignHorizontalCenterWithPanelConstraint(true);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
    panelSize = graph.CalculateDesiredSize();
    VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

    VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
    VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
    VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
    VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
    VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
    VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
    VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
    VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
}

void RelativePanelUnitTests::VerifyAboveAndOutOfPanel()
{
    const XSIZEF availableSize{ XFLOAT_INF, XFLOAT_INF };
    const XRECTF finalRect{ 0, 0, 100, 100 };
    const XRECTF mr0{ 0, 0, 100, 100 };
    const XRECTF mr1{ 0, 0, 100, 0 };
    const XRECTF ar0{ 0, 0, 100, 100 };
    const XRECTF ar1{ 0, 0, 100, 0 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);

    node0->SetAlignTopWithPanelConstraint(true);
    node1->SetAboveConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
    panelSize = graph.CalculateDesiredSize();
    VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

    VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
    VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
    VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
    VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
    VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
    VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
}

void RelativePanelUnitTests::VerifyBelowAndOutOfPanel()
{
    const XSIZEF availableSize{ XFLOAT_INF, XFLOAT_INF };
    const XRECTF finalRect{ 0, 0, 100, 100 };
    const XRECTF mr0{ 0, 0, 100, 100 };
    const XRECTF mr1{ 0, 100, 100, 0 };
    const XRECTF ar0{ 0, 0, 100, 100 };
    const XRECTF ar1{ 0, 100, 100, 0 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);

    node0->SetAlignBottomWithPanelConstraint(true);
    node1->SetBelowConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
    panelSize = graph.CalculateDesiredSize();
    VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

    VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
    VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
    VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
    VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
    VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
    VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
}

void RelativePanelUnitTests::VerifyLeftAndOutOfPanel()
{
    const XSIZEF availableSize{ XFLOAT_INF, XFLOAT_INF };
    const XRECTF finalRect{ 0, 0, 100, 100 };
    const XRECTF mr0{ 0, 0, 100, 100 };
    const XRECTF mr1{ 0, 0, 0, 100 };
    const XRECTF ar0{ 0, 0, 100, 100 };
    const XRECTF ar1{ 0, 0, 0, 100 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);

    node0->SetAlignLeftWithPanelConstraint(true);
    node1->SetLeftOfConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
    panelSize = graph.CalculateDesiredSize();
    VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

    VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
    VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
    VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
    VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
    VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
    VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
}

void RelativePanelUnitTests::VerifyRightAndOutOfPanel()
{
    const XSIZEF availableSize{ XFLOAT_INF, XFLOAT_INF };
    const XRECTF finalRect{ 0, 0, 100, 100 };
    const XRECTF mr0{ 0, 0, 100, 100 };
    const XRECTF mr1{ 100, 0, 0, 100 };
    const XRECTF ar0{ 0, 0, 100, 100 };
    const XRECTF ar1{ 100, 0, 0, 100 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);

    node0->SetAlignRightWithPanelConstraint(true);
    node1->SetRightOfConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
    panelSize = graph.CalculateDesiredSize();
    VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

    VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
    VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
    VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
    VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
    VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
    VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
}

void RelativePanelUnitTests::VerifyVerticalMultiDirectionalLeaf()
{
    const XSIZEF availableSize{ 2000, 2000 };

    {
        const XRECTF finalRect{ 0, 0, 100, 550 };
        const XRECTF mr0{ 0, 0, 100, 550 };
        const XRECTF mr1{ 0, 0, 100, 550 };
        const XRECTF mr2{ 0, 100, 100, 250 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 350, 100, 200 };
        const XRECTF ar2{ 0, 100, 100, 250 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 250);

        node0->SetAlignTopWithPanelConstraint(true);
        node1->SetAlignBottomWithPanelConstraint(true);
        node2->SetBelowConstraint(node0);
        node2->SetAboveConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 450 };
        const XRECTF mr0{ 0, 0, 100, 450 };
        const XRECTF mr1{ 0, 0, 100, 450 };
        const XRECTF mr2{ 0, 100, 100, 150 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 250, 100, 200 };
        const XRECTF ar2{ 0, 100, 100, 150 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 150);

        node0->SetAlignTopWithPanelConstraint(true);
        node1->SetAlignBottomWithPanelConstraint(true);
        node2->SetBelowConstraint(node0);
        node2->SetAboveConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 350 };
        const XRECTF mr0{ 0, 0, 100, 350 };
        const XRECTF mr1{ 0, 0, 100, 350 };
        const XRECTF mr2{ 0, 100, 100, 50 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 150, 100, 200 };
        const XRECTF ar2{ 0, 100, 100, 50 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 50);

        node0->SetAlignTopWithPanelConstraint(true);
        node1->SetAlignBottomWithPanelConstraint(true);
        node2->SetBelowConstraint(node0);
        node2->SetAboveConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifyHorizontalMultiDirectionalLeaf()
{
    const XSIZEF availableSize{ 2000, 2000 };

    {
        const XRECTF finalRect{ 0, 0, 550, 100 };
        const XRECTF mr0{ 0, 0, 550, 100 };
        const XRECTF mr1{ 0, 0, 550, 100 };
        const XRECTF mr2{ 100, 0, 250, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 350, 0, 200, 100 };
        const XRECTF ar2{ 100, 0, 250, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 250, 100);

        node0->SetAlignLeftWithPanelConstraint(true);
        node1->SetAlignRightWithPanelConstraint(true);
        node2->SetRightOfConstraint(node0);
        node2->SetLeftOfConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 450, 100 };
        const XRECTF mr0{ 0, 0, 450, 100 };
        const XRECTF mr1{ 0, 0, 450, 100 };
        const XRECTF mr2{ 100, 0, 150, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 250, 0, 200, 100 };
        const XRECTF ar2{ 100, 0, 150, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 150, 100);

        node0->SetAlignLeftWithPanelConstraint(true);
        node1->SetAlignRightWithPanelConstraint(true);
        node2->SetRightOfConstraint(node0);
        node2->SetLeftOfConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 350, 100 };
        const XRECTF mr0{ 0, 0, 350, 100 };
        const XRECTF mr1{ 0, 0, 350, 100 };
        const XRECTF mr2{ 100, 0, 50, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 150, 0, 200, 100 };
        const XRECTF ar2{ 100, 0, 50, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 50, 100);

        node0->SetAlignLeftWithPanelConstraint(true);
        node1->SetAlignRightWithPanelConstraint(true);
        node2->SetRightOfConstraint(node0);
        node2->SetLeftOfConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifyAboveVerticallyCenteredRoot()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);

    node0->SetAlignVerticalCenterWithPanelConstraint(true);
    node1->SetAlignBottomWithConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));

    {
        const XRECTF finalRect{ 0, 0, 100, 300 };
        const XRECTF mr0{ 0, 0, 100, 300 };
        const XRECTF mr1{ 0, 0, 100, 200 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 200 };

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 500 };
        const XRECTF mr0{ 0, 0, 100, 500 };
        const XRECTF mr1{ 0, 0, 100, 200 };
        const XRECTF ar0{ 0, 200, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 200 };

        node1->SetAlignBottomWithConstraint(nullptr);
        node1->SetAboveConstraint(node0);

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }
}

void RelativePanelUnitTests::VerifyBelowVerticallyCenteredRoot()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);

    node0->SetAlignVerticalCenterWithPanelConstraint(true);
    node1->SetAlignTopWithConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));

    {
        const XRECTF finalRect{ 0, 0, 100, 300 };
        const XRECTF mr0{ 0, 0, 100, 300 };
        const XRECTF mr1{ 0, 100, 100, 200 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 200 };

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }

    {
        const XRECTF finalRect{ 0, 0, 100, 500 };
        const XRECTF mr0{ 0, 0, 100, 500 };
        const XRECTF mr1{ 0, 300, 100, 200 };
        const XRECTF ar0{ 0, 200, 100, 100 };
        const XRECTF ar1{ 0, 300, 100, 200 };

        node1->SetAlignTopWithConstraint(nullptr);
        node1->SetBelowConstraint(node0);

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }
}

void RelativePanelUnitTests::VerifyLeftOfHorizontallyCenteredRoot()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);

    node0->SetAlignHorizontalCenterWithPanelConstraint(true);
    node1->SetAlignRightWithConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));

    {
        const XRECTF finalRect{ 0, 0, 300, 100 };
        const XRECTF mr0{ 0, 0, 300, 100 };
        const XRECTF mr1{ 0, 0, 200, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 200, 100 };

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }

    {
        const XRECTF finalRect{ 0, 0, 500, 100 };
        const XRECTF mr0{ 0, 0, 500, 100 };
        const XRECTF mr1{ 0, 0, 200, 100 };
        const XRECTF ar0{ 200, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 200, 100 };

        node1->SetAlignRightWithConstraint(nullptr);
        node1->SetLeftOfConstraint(node0);

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }
}

void RelativePanelUnitTests::VerifyRightOfHorizontallyCenteredRoot()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);

    node0->SetAlignHorizontalCenterWithPanelConstraint(true);
    node1->SetAlignLeftWithConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));

    {
        const XRECTF finalRect{ 0, 0, 300, 100 };
        const XRECTF mr0{ 0, 0, 300, 100 };
        const XRECTF mr1{ 100, 0, 200, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 200, 100 };

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }

    {
        const XRECTF finalRect{ 0, 0, 500, 100 };
        const XRECTF mr0{ 0, 0, 500, 100 };
        const XRECTF mr1{ 300, 0, 200, 100 };
        const XRECTF ar0{ 200, 0, 100, 100 };
        const XRECTF ar1{ 300, 0, 200, 100 };

        node1->SetAlignLeftWithConstraint(nullptr);
        node1->SetRightOfConstraint(node0);

        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
    }
}

void RelativePanelUnitTests::VerifyVerticallyCenteredChains()
{
    const XSIZEF availableSize{ 2000, 2000 };
    {
        const XRECTF finalRect{ 0, 0, 200, 300 };
        const XRECTF mr0{ 0, 0, 200, 300 };
        const XRECTF mr1{ 0, 0, 200, 100 };
        const XRECTF mr2{ 100, 0, 100, 300 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 100, 0, 100, 300 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 300);

        node0->SetAlignVerticalCenterWithPanelConstraint(true);
        node1->SetAboveConstraint(node0);
        node2->SetAlignTopWithConstraint(node1);
        node2->SetRightOfConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 300 };
        const XRECTF mr0{ 0, 0, 200, 300 };
        const XRECTF mr1{ 0, 200, 200, 100 };
        const XRECTF mr2{ 100, 0, 100, 300 };
        const XRECTF ar0{ 0, 100, 100, 100 };
        const XRECTF ar1{ 0, 200, 100, 100 };
        const XRECTF ar2{ 100, 0, 100, 300 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 300);

        node0->SetAlignVerticalCenterWithPanelConstraint(true);
        node1->SetBelowConstraint(node0);
        node2->SetAlignBottomWithConstraint(node1);
        node2->SetRightOfConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifyHorizontallyCenteredChains()
{
    const XSIZEF availableSize{ 2000, 2000 };
    {
        const XRECTF finalRect{ 0, 0, 300, 200 };
        const XRECTF mr0{ 0, 0, 300, 200 };
        const XRECTF mr1{ 0, 0, 100, 200 };
        const XRECTF mr2{ 0, 100, 300, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 0, 0, 100, 100 };
        const XRECTF ar2{ 0, 100, 300, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 300, 100);

        node0->SetAlignHorizontalCenterWithPanelConstraint(true);
        node1->SetLeftOfConstraint(node0);
        node2->SetAlignLeftWithConstraint(node1);
        node2->SetBelowConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }

    {
        const XRECTF finalRect{ 0, 0, 300, 200 };
        const XRECTF mr0{ 0, 0, 300, 200 };
        const XRECTF mr1{ 200, 0, 100, 200 };
        const XRECTF mr2{ 0, 100, 300, 100 };
        const XRECTF ar0{ 100, 0, 100, 100 };
        const XRECTF ar1{ 200, 0, 100, 100 };
        const XRECTF ar2{ 0, 100, 300, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 300, 100);

        node0->SetAlignHorizontalCenterWithPanelConstraint(true);
        node1->SetRightOfConstraint(node0);
        node2->SetAlignRightWithConstraint(node1);
        node2->SetBelowConstraint(node1);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
    }
}

void RelativePanelUnitTests::VerifySimultaneousTopAndBottomAlignment()
{
    const XSIZEF availableSize{ 2000, 2000 };

    {
        const XRECTF finalRect{ 0, 0, 200, 400 };
        const XRECTF mr0{ 0, 0, 200, 400 };
        const XRECTF mr1{ 0, 100, 200, 300 };
        const XRECTF mr2{ 100, 100, 100, 200 };
        const XRECTF mr3{ 100, 300, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 200 };
        const XRECTF ar2{ 100, 100, 100, 200 };
        const XRECTF ar3{ 100, 300, 100, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);

        node0->SetAlignTopWithPanelConstraint(true);
        node1->SetBelowConstraint(node0);
        node2->SetRightOfConstraint(node1);
        node2->SetAlignTopWithConstraint(node1);
        node2->SetAlignBottomWithConstraint(node1);
        node3->SetBelowConstraint(node2);
        node3->SetAlignLeftWithConstraint(node2);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 400 };
        const XRECTF mr0{ 0, 0, 200, 400 };
        const XRECTF mr1{ 0, 0, 200, 300 };
        const XRECTF mr2{ 100, 100, 100, 200 };
        const XRECTF mr3{ 100, 0, 100, 100 };
        const XRECTF ar0{ 0, 300, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 200 };
        const XRECTF ar2{ 100, 100, 100, 200 };
        const XRECTF ar3{ 100, 0, 100, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);

        node0->SetAlignBottomWithPanelConstraint(true);
        node1->SetAboveConstraint(node0);
        node2->SetRightOfConstraint(node1);
        node2->SetAlignTopWithConstraint(node1);
        node2->SetAlignBottomWithConstraint(node1);
        node3->SetAboveConstraint(node2);
        node3->SetAlignLeftWithConstraint(node2);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 400 };
        const XRECTF mr0{ 0, 0, 200, 400 };
        const XRECTF mr1{ 0, 100, 200, 300 };
        const XRECTF mr2{ 0, 200, 200, 200 };
        const XRECTF mr3{ 100, 100, 100, 200 };
        const XRECTF mr4{ 100, 300, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 200, 100, 100 };
        const XRECTF ar3{ 100, 100, 100, 200 };
        const XRECTF ar4{ 100, 300, 100, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3, e4;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);
        RPNode* node4 = AddNodeToGraph(&graph, &e4, 100, 100);

        node0->SetAlignTopWithPanelConstraint(true);
        node1->SetBelowConstraint(node0);
        node2->SetBelowConstraint(node1);
        node3->SetRightOfConstraint(node1);
        node3->SetAlignTopWithConstraint(node1);
        node3->SetAlignBottomWithConstraint(node2);
        node4->SetBelowConstraint(node3);
        node4->SetAlignLeftWithConstraint(node3);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
        VERIFY_ARE_EQUAL(node4->m_measureRect, mr4);
        VERIFY_ARE_EQUAL(node4->m_arrangeRect, ar4);
    }
}

void RelativePanelUnitTests::VerifySimultaneousLeftAndRightAlignment()
{
    const XSIZEF availableSize{ 2000, 2000 };
    {
        const XRECTF finalRect{ 0, 0, 400, 200 };
        const XRECTF mr0{ 0, 0, 400, 200 };
        const XRECTF mr1{ 100, 0, 300, 200 };
        const XRECTF mr2{ 100, 100, 200, 100 };
        const XRECTF mr3{ 300, 100, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 200, 100 };
        const XRECTF ar2{ 100, 100, 200, 100 };
        const XRECTF ar3{ 300, 100, 100, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);

        node0->SetAlignLeftWithPanelConstraint(true);
        node1->SetRightOfConstraint(node0);
        node2->SetBelowConstraint(node1);
        node2->SetAlignLeftWithConstraint(node1);
        node2->SetAlignRightWithConstraint(node1);
        node3->SetRightOfConstraint(node2);
        node3->SetAlignTopWithConstraint(node2);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }

    {
        const XRECTF finalRect{ 0, 0, 400, 200 };
        const XRECTF mr0{ 0, 0, 400, 200 };
        const XRECTF mr1{ 0, 0, 300, 200 };
        const XRECTF mr2{ 100, 100, 200, 100 };
        const XRECTF mr3{ 0, 100, 100, 100 };
        const XRECTF ar0{ 300, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 200, 100 };
        const XRECTF ar2{ 100, 100, 200, 100 };
        const XRECTF ar3{ 0, 100, 100, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);

        node0->SetAlignRightWithPanelConstraint(true);
        node1->SetLeftOfConstraint(node0);
        node2->SetBelowConstraint(node1);
        node2->SetAlignLeftWithConstraint(node1);
        node2->SetAlignRightWithConstraint(node1);
        node3->SetLeftOfConstraint(node2);
        node3->SetAlignTopWithConstraint(node2);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }

    {
        const XRECTF finalRect{ 0, 0, 400, 200 };
        const XRECTF mr0{ 0, 0, 400, 200 };
        const XRECTF mr1{ 100, 0, 300, 200 };
        const XRECTF mr2{ 200, 0, 200, 200 };
        const XRECTF mr3{ 100, 100, 200, 100 };
        const XRECTF mr4{ 300, 100, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 200, 0, 100, 100 };
        const XRECTF ar3{ 100, 100, 200, 100 };
        const XRECTF ar4{ 300, 100, 100, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3, e4;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);
        RPNode* node4 = AddNodeToGraph(&graph, &e4, 100, 100);

        node0->SetAlignLeftWithPanelConstraint(true);
        node1->SetRightOfConstraint(node0);
        node2->SetRightOfConstraint(node1);
        node3->SetBelowConstraint(node1);
        node3->SetAlignLeftWithConstraint(node1);
        node3->SetAlignRightWithConstraint(node2);
        node4->SetRightOfConstraint(node3);
        node4->SetAlignTopWithConstraint(node3);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
        VERIFY_ARE_EQUAL(node4->m_measureRect, mr4);
        VERIFY_ARE_EQUAL(node4->m_arrangeRect, ar4);
    }
}

void RelativePanelUnitTests::VerifySimultaneousTopAndBottomAlignmentToChains()
{
    const XSIZEF availableSize{ 2000, 2000 };

    {
        const XRECTF finalRect{ 0, 0, 300, 600 };
        const XRECTF mr0{ 0, 0, 300, 600 };
        const XRECTF mr1{ 0, 100, 300, 500 };
        const XRECTF mr2{ 0, 0, 300, 600 };
        const XRECTF mr3{ 0, 0, 300, 500 };
        const XRECTF mr4{ 100, 100, 200, 200 };
        const XRECTF mr5{ 100, 0, 200, 500 };
        const XRECTF mr6{ 200, 100, 100, 400 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 500, 100, 100 };
        const XRECTF ar3{ 0, 400, 100, 100 };
        const XRECTF ar4{ 100, 100, 100, 200 };
        const XRECTF ar5{ 100, 300, 100, 200 };
        const XRECTF ar6{ 200, 100, 100, 400 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3, e4, e5, e6;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);
        RPNode* node4 = AddNodeToGraph(&graph, &e4, 100, 200);
        RPNode* node5 = AddNodeToGraph(&graph, &e5, 100, 200);
        RPNode* node6 = AddNodeToGraph(&graph, &e6, 100, 400);

        node0->SetAlignTopWithPanelConstraint(true);
        node1->SetBelowConstraint(node0);
        node2->SetAlignBottomWithPanelConstraint(true);
        node3->SetAboveConstraint(node2);
        node4->SetAlignTopWithConstraint(node1);
        node4->SetRightOfConstraint(node1);
        node4->SetAboveConstraint(node5);
        node5->SetRightOfConstraint(node3);
        node5->SetAlignBottomWithConstraint(node3);
        node6->SetAlignTopWithConstraint(node4);
        node6->SetAlignBottomWithConstraint(node5);
        node6->SetRightOfConstraint(node4);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
        VERIFY_ARE_EQUAL(node4->m_measureRect, mr4);
        VERIFY_ARE_EQUAL(node4->m_arrangeRect, ar4);
        VERIFY_ARE_EQUAL(node5->m_measureRect, mr5);
        VERIFY_ARE_EQUAL(node5->m_arrangeRect, ar5);
        VERIFY_ARE_EQUAL(node6->m_measureRect, mr6);
        VERIFY_ARE_EQUAL(node6->m_arrangeRect, ar6);
    }

    {
        const XRECTF finalRect{ 0, 0, 300, 600 };
        const XRECTF mr0{ 0, 0, 300, 600 };
        const XRECTF mr1{ 0, 100, 300, 500 };
        const XRECTF mr2{ 0, 0, 300, 600 };
        const XRECTF mr3{ 0, 0, 300, 500 };
        const XRECTF mr4{ 100, 100, 200, 400 };
        const XRECTF mr5{ 200, 100, 100, 400 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 0, 100, 100, 100 };
        const XRECTF ar2{ 0, 500, 100, 100 };
        const XRECTF ar3{ 0, 400, 100, 100 };
        const XRECTF ar4{ 100, 100, 100, 400 };
        const XRECTF ar5{ 200, 100, 100, 400 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3, e4, e5;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);
        RPNode* node4 = AddNodeToGraph(&graph, &e4, 100, 400);
        RPNode* node5 = AddNodeToGraph(&graph, &e5, 100, 400);

        node0->SetAlignTopWithPanelConstraint(true);
        node1->SetBelowConstraint(node0);
        node2->SetAlignBottomWithPanelConstraint(true);
        node3->SetAboveConstraint(node2);
        node4->SetAlignTopWithConstraint(node1);
        node4->SetAlignBottomWithConstraint(node3);
        node4->SetRightOfConstraint(node1);
        node5->SetAlignTopWithConstraint(node4);
        node5->SetAlignBottomWithConstraint(node4);
        node5->SetRightOfConstraint(node4);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
        VERIFY_ARE_EQUAL(node4->m_measureRect, mr4);
        VERIFY_ARE_EQUAL(node4->m_arrangeRect, ar4);
        VERIFY_ARE_EQUAL(node5->m_measureRect, mr5);
        VERIFY_ARE_EQUAL(node5->m_arrangeRect, ar5);
    }
}

void RelativePanelUnitTests::VerifySimultaneousLeftAndRightAlignmentToChains()
{
    const XSIZEF availableSize{ 2000, 2000 };

    {
        const XRECTF finalRect{ 0, 0, 600, 300 };
        const XRECTF mr0{ 0, 0, 600, 300 };
        const XRECTF mr1{ 100, 0, 500, 300 };
        const XRECTF mr2{ 0, 0, 600, 300 };
        const XRECTF mr3{ 0, 0, 500, 300 };
        const XRECTF mr4{ 100, 100, 200, 200 };
        const XRECTF mr5{ 0, 100, 500, 200 };
        const XRECTF mr6{ 100, 200, 400, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 500, 0, 100, 100 };
        const XRECTF ar3{ 400, 0, 100, 100 };
        const XRECTF ar4{ 100, 100, 200, 100 };
        const XRECTF ar5{ 300, 100, 200, 100 };
        const XRECTF ar6{ 100, 200, 400, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3, e4, e5, e6;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);
        RPNode* node4 = AddNodeToGraph(&graph, &e4, 200, 100);
        RPNode* node5 = AddNodeToGraph(&graph, &e5, 200, 100);
        RPNode* node6 = AddNodeToGraph(&graph, &e6, 400, 100);

        node0->SetAlignLeftWithPanelConstraint(true);
        node1->SetRightOfConstraint(node0);
        node2->SetAlignRightWithPanelConstraint(true);
        node3->SetLeftOfConstraint(node2);
        node4->SetAlignLeftWithConstraint(node1);
        node4->SetBelowConstraint(node1);
        node4->SetLeftOfConstraint(node5);
        node5->SetBelowConstraint(node3);
        node5->SetAlignRightWithConstraint(node3);
        node6->SetAlignLeftWithConstraint(node4);
        node6->SetAlignRightWithConstraint(node5);
        node6->SetBelowConstraint(node4);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
        VERIFY_ARE_EQUAL(node4->m_measureRect, mr4);
        VERIFY_ARE_EQUAL(node4->m_arrangeRect, ar4);
        VERIFY_ARE_EQUAL(node5->m_measureRect, mr5);
        VERIFY_ARE_EQUAL(node5->m_arrangeRect, ar5);
        VERIFY_ARE_EQUAL(node6->m_measureRect, mr6);
        VERIFY_ARE_EQUAL(node6->m_arrangeRect, ar6);
    }

    {
        const XRECTF finalRect{ 0, 0, 600, 300 };
        const XRECTF mr0{ 0, 0, 600, 300 };
        const XRECTF mr1{ 100, 0, 500, 300 };
        const XRECTF mr2{ 0, 0, 600, 300 };
        const XRECTF mr3{ 0, 0, 500, 300 };
        const XRECTF mr4{ 100, 100, 400, 200 };
        const XRECTF mr5{ 100, 200, 400, 100 };
        const XRECTF ar0{ 0, 0, 100, 100 };
        const XRECTF ar1{ 100, 0, 100, 100 };
        const XRECTF ar2{ 500, 0, 100, 100 };
        const XRECTF ar3{ 400, 0, 100, 100 };
        const XRECTF ar4{ 100, 100, 400, 100 };
        const XRECTF ar5{ 100, 200, 400, 100 };

        RPGraph graph;
        CUIElement e0, e1, e2, e3, e4, e5;
        XSIZEF panelSize;

        RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
        RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);
        RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 100);
        RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);
        RPNode* node4 = AddNodeToGraph(&graph, &e4, 400, 100);
        RPNode* node5 = AddNodeToGraph(&graph, &e5, 400, 100);

        node0->SetAlignLeftWithPanelConstraint(true);
        node1->SetRightOfConstraint(node0);
        node2->SetAlignRightWithPanelConstraint(true);
        node3->SetLeftOfConstraint(node2);
        node4->SetAlignLeftWithConstraint(node1);
        node4->SetAlignRightWithConstraint(node3);
        node4->SetBelowConstraint(node1);
        node5->SetAlignLeftWithConstraint(node4);
        node5->SetAlignRightWithConstraint(node4);
        node5->SetBelowConstraint(node4);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
        VERIFY_ARE_EQUAL(node4->m_measureRect, mr4);
        VERIFY_ARE_EQUAL(node4->m_arrangeRect, ar4);
        VERIFY_ARE_EQUAL(node5->m_measureRect, mr5);
        VERIFY_ARE_EQUAL(node5->m_arrangeRect, ar5);
    }
}

void RelativePanelUnitTests::VerifyChainWithHorizontallyCenteredSiblings()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1, e2, e3;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 200, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 200, 100);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 300, 100);
    RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);

    node0->SetAlignTopWithPanelConstraint(true);
    node0->SetAlignLeftWithPanelConstraint(true);
    node1->SetRightOfConstraint(node0);
    node2->SetBelowConstraint(node1);
    node2->SetAlignHorizontalCenterWithConstraint(node1);
    node3->SetLeftOfConstraint(node2);
    node3->SetAlignTopWithConstraint(node2);

    {
        const XRECTF finalRect{ 0, 0, 450, 200 };
        const XRECTF mr0{ 0, 0, 450, 200 };
        const XRECTF mr1{ 200, 0, 250, 200 };
        const XRECTF mr2{ 150, 100, 300, 100 };
        const XRECTF mr3{ 0, 100, 150, 100 };
        const XRECTF ar0{ 0, 0, 200, 100 };
        const XRECTF ar1{ 200, 0, 200, 100 };
        const XRECTF ar2{ 150, 100, 300, 100 };
        const XRECTF ar3{ 50, 100, 100, 100 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }

    {
        const XRECTF finalRect{ 0, 0, 550, 200 };
        const XRECTF mr0{ 0, 0, 550, 200 };
        const XRECTF mr1{ 200, 0, 350, 200 };
        const XRECTF mr2{ 50, 100, 500, 100 };
        const XRECTF mr3{ 450, 100, 100, 100 };
        const XRECTF ar0{ 0, 0, 200, 100 };
        const XRECTF ar1{ 200, 0, 200, 100 };
        const XRECTF ar2{ 150, 100, 300, 100 };
        const XRECTF ar3{ 450, 100, 100, 100 };

        node3->SetLeftOfConstraint(nullptr);
        node3->SetRightOfConstraint(node2);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }
}

void RelativePanelUnitTests::VerifyChainWithVerticallyCenteredSiblings()
{
    const XSIZEF availableSize{ 2000, 2000 };

    RPGraph graph;
    CUIElement e0, e1, e2, e3;
    XSIZEF panelSize;

    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 200);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 200);
    RPNode* node2 = AddNodeToGraph(&graph, &e2, 100, 300);
    RPNode* node3 = AddNodeToGraph(&graph, &e3, 100, 100);

    node0->SetAlignTopWithPanelConstraint(true);
    node0->SetAlignLeftWithPanelConstraint(true);
    node1->SetBelowConstraint(node0);
    node2->SetRightOfConstraint(node1);
    node2->SetAlignVerticalCenterWithConstraint(node1);
    node3->SetAboveConstraint(node2);
    node3->SetAlignLeftWithConstraint(node2);

    {
        const XRECTF finalRect{ 0, 0, 200, 450 };
        const XRECTF mr0{ 0, 0, 200, 450 };
        const XRECTF mr1{ 0, 200, 200, 250 };
        const XRECTF mr2{ 100, 150, 100, 300 };
        const XRECTF mr3{ 100, 0, 100, 150 };
        const XRECTF ar0{ 0, 0, 100, 200 };
        const XRECTF ar1{ 0, 200, 100, 200 };
        const XRECTF ar2{ 100, 150, 100, 300 };
        const XRECTF ar3{ 100, 50, 100, 100 };

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }

    {
        const XRECTF finalRect{ 0, 0, 200, 550 };
        const XRECTF mr0{ 0, 0, 200, 550 };
        const XRECTF mr1{ 0, 200, 200, 350 };
        const XRECTF mr2{ 100, 50, 100, 500 };
        const XRECTF mr3{ 100, 450, 100, 100 };
        const XRECTF ar0{ 0, 0, 100, 200 };
        const XRECTF ar1{ 0, 200, 100, 200 };
        const XRECTF ar2{ 100, 150, 100, 300 };
        const XRECTF ar3{ 100, 450, 100, 100 };

        node3->SetAboveConstraint(nullptr);
        node3->SetBelowConstraint(node2);

        VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
        panelSize = graph.CalculateDesiredSize();
        VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

        VERIFY_ARE_EQUAL(panelSize.width, finalRect.Width);
        VERIFY_ARE_EQUAL(panelSize.height, finalRect.Height);
        VERIFY_ARE_EQUAL(node0->m_measureRect, mr0);
        VERIFY_ARE_EQUAL(node0->m_arrangeRect, ar0);
        VERIFY_ARE_EQUAL(node1->m_measureRect, mr1);
        VERIFY_ARE_EQUAL(node1->m_arrangeRect, ar1);
        VERIFY_ARE_EQUAL(node2->m_measureRect, mr2);
        VERIFY_ARE_EQUAL(node2->m_arrangeRect, ar2);
        VERIFY_ARE_EQUAL(node3->m_measureRect, mr3);
        VERIFY_ARE_EQUAL(node3->m_arrangeRect, ar3);
    }
}

void RelativePanelUnitTests::VerifyPhysicallyImpossibleDefinitions()
{
    const XSIZEF availableSize{ 2000, 2000 };
    const XRECTF finalRect{ 0, 0, 200, 100 };

    RPGraph graph;
    CUIElement e0, e1;
    
    RPNode* node0 = AddNodeToGraph(&graph, &e0, 100, 100);
    RPNode* node1 = AddNodeToGraph(&graph, &e1, 100, 100);

    node0->SetAlignTopWithPanelConstraint(true);
    node1->SetRightOfConstraint(node0);
    node1->SetLeftOfConstraint(node0);
    node1->SetAboveConstraint(node0);
    node1->SetBelowConstraint(node0);

    VERIFY_SUCCEEDED(graph.MeasureNodes(availableSize));
    VERIFY_SUCCEEDED(graph.ArrangeNodes(finalRect));

    VERIFY_IS_LESS_THAN(node1->m_measureRect.Width, 0);
    VERIFY_IS_LESS_THAN(node1->m_arrangeRect.Width, 0);
    VERIFY_IS_LESS_THAN(node1->m_measureRect.Height, 0);
    VERIFY_IS_LESS_THAN(node1->m_arrangeRect.Height, 0);
}

} } } } }