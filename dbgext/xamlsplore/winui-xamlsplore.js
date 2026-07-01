"use strict";

function GetTLSPointer(thread, module, variableName)
{
    try
    {
        var tlsSlot = host.getModuleSymbol(module, variableName, "int");

        if (tlsSlot < 64)
        {
            return thread.Environment.EnvironmentBlock.TlsSlots[tlsSlot];
        }
        else
        {
            return thread.Environment.EnvironmentBlock.TlsExpansionSlots[tlsSlot - 64];
        }
    }
    catch (ex)
    {
    }
    return null;
}

var XAML_DLL = undefined;

function XAML_DLL_Name()
{
    if (XAML_DLL == undefined)
    {
        if (host.currentProcess.Modules.Where(function(m) { return m.Name.toLowerCase().endsWith("microsoft.ui.xaml.dll") }).Count() != 0)
        {
            XAML_DLL = "microsoft.ui.xaml.dll";
            try
            {
                var v = host.getModuleSymbol(XAML_DLL, "DXamlInstanceStorage::g_dwTlsIndex", "int");
            }
            catch (exp)
            {
                XAML_DLL = undefined;
            }
        }
    }
    return XAML_DLL;
}

function invokeScript()
{
    //
    // Insert your script content here. This method will be called whenever the script is
    // invoked from a client.
    //
    // See the following for more details:
    //
    //     See debugger JavaScript extension documentation.

    // Debugging helper:
    //host.diagnostics.logUnhandledExceptions = true;
}

function __debugDump(o)
{
    if (o == undefined || o == null)
    {
        host.diagnostics.debugLog("Dumping " + o + "\n");
        return;
    }

    host.diagnostics.debugLog("Dumping " + o + " (targetType=" + o.targetType + ")\n");
    try
    {
        for (var fldName of Object.getOwnPropertyNames(o))
        {
            try
            {
                host.diagnostics.debugLog("  " + fldName + ": " + o[fldName] + "\n");
            }
            catch (e)
            {
                host.diagnostics.debugLog("  " + fldName + " error: " + e + "\n");
            }
        }
        host.diagnostics.debugLog("  ....\n");
    }
    catch (e) {}

    try
    {
        for (var v in o)
        {
            try
            {
                host.diagnostics.debugLog("  " + v + ": " + o[v] + "\n");
            }
            catch (e)
            {
                host.diagnostics.debugLog("  " + v + " error: " + e + "\n");
            }
        }
    }
    catch (e) {}

    host.diagnostics.debugLog("  done.\n");
}

function __IsXAMLLoaded()
{
    return (host.currentProcess.Modules.Where(function(m) { return m.Name.toLowerCase().endsWith(XAML_DLL_Name()) }).Count() != 0);
}

function __CheckXAMLSymbolsLoaded()
{
    if (!__IsXAMLLoaded())
    {
        throw new Error("XAML is not loaded.");
        //host.diagnostics.debugLog("XAML is not loaded.\n");
        return false;
    }

    // Note: I think will break when host.apiVersionSupport(1,1) is used to get new behavior where
    //       getModuleSymbol returns null for unknown symbols.
    try
    {
        var v = host.getModuleSymbol(XAML_DLL_Name(), "DXamlInstanceStorage::g_dwTlsIndex", "int");
    }
    catch (exp)
    {
        if (exp.toString().indexOf("Invalid argument to method 'getModuleSymbol'") > 0)
        {
            throw new Error("Symbols for XAML not loaded/unavailable.");
            //host.diagnostics.debugLog("Symbols for XAML not loaded/unavailable.\n");
            return false;
        }
        // else the error might be lack of heap or something else.
    }

    return true;
}

function dumpStartHTML(outputLines)
{
    var s = String.raw`
<!DOCTYPE html>
<html>

<script src="https://ajax.aspnetcdn.com/ajax/jQuery/jquery-3.3.1.min.js"></script>

<script>

  ///////////////////////////////////////////////////////////
  //
  // Utilities
  //
  ///////////////////////////////////////////////////////////

  function isDefined(value) {
    return value !== undefined;
  }

  function isCloseReal(num1, num2) {
    const epsilon = 0.001;
    return Math.abs(num1 - num2) < epsilon;
  }

  function arraysAreSame(a1, a2) {
    if (a1.length !== a2.length) {
      return false;
    }

    for (const i in a1) {
      if (a1[i] !== a2[i]) {
        return false;
      }
    }

    return true;
  }

  function addNodeWarning(node, text, anchor) {
    var text = '!<a href="#' + anchor + '">'
      + text + '</a>';

    if (!isDefined(node._warnings)) {
      node._warnings = [];
    }

    node._warnings.push(text);
  }

  function addNodeNote(node, text) {
    if (!isDefined(node._notes)) {
      node._notes = [];
    }

    if (!node._notes.includes(text)) {
      node._notes.push(text);
    }
  }

  function mapLocalBoundsToWorld(bounds, visual) {
    // The bounds to be mapped is in the local space of the visual.
    // The challenge is to get it to to world space via the same
    // transform that took the visuals' m_Bounds to the computed
    // bounds* properties.  There are two possibilities:  Infer the
    // transform from these 2 bounds rects.  The problem with this
    // is that m_Bounds is in parent space, with the transform on
    // the visual already applied.  The inferred transform would be
    // missing the visual's own transform.  We could make up for this
    // by grabbing and multiplying in the visual's own transform.
    // The other method is to take the world transform from the visual's
    // tree data.  We hope this is up-to-date because this route is
    // much simpler.
    return visual._worldTransform.transformRect(bounds);
  }

  var Matrix4x4 = function() {
  };

  Matrix4x4.prototype = {
    transformPoint2 : function(x, y) {
      var xo = this.c0[0] * x + this.c0[1] * y + this.c0[3];
      var yo = this.c1[0] * x + this.c1[1] * y + this.c1[3];
      var w  = this.c3[0] * x + this.c3[1] * y + this.c3[3];
      if (!isCloseReal(w, 1.0) && !isCloseReal(w, 0.0)) {
        xo = xo / w;
        yo = yo / w;
      }

      return [xo, yo];
    },

    transformRect : function(rect) {
      var x = [];
      var y = [];
      [x[0], y[0]] = this.transformPoint2(rect.left, rect.top);
      [x[1], y[1]] = this.transformPoint2(rect.left, rect.bottom);
      [x[2], y[2]] = this.transformPoint2(rect.right, rect.top);
      [x[3], y[3]] = this.transformPoint2(rect.right, rect.bottom);
  
      x.sort((a, b) => a - b);
      y.sort((a, b) => a - b);
      return Rect.fromLTRB(x[0], y[0], x[3], y[3]);
    }
  };

  Matrix4x4.fromVisualTreeData = function(visual) {
    const transform = visual.properties.m_desktopTreeData.m_worldTransform.m_float4x4;
    var mat = new Matrix4x4;
    mat.c0 = [Number(transform.m11), Number(transform.m21), Number(transform.m31), Number(transform.m41)];
    mat.c1 = [Number(transform.m12), Number(transform.m22), Number(transform.m32), Number(transform.m42)];
    mat.c2 = [Number(transform.m13), Number(transform.m23), Number(transform.m33), Number(transform.m43)];
    mat.c3 = [Number(transform.m14), Number(transform.m24), Number(transform.m34), Number(transform.m44)];
    return mat;
  }


  var Rect = function() {
  }

  Rect.prototype = {
    width : function() {
      return this.right - this.left;
    },

    height : function() {
      return this.bottom - this.top;
    },

    isEmpty : function() {
      return (this.width() <= 0.0) || (this.height() <= 0.0);
    },

    isInfinite : function() {
      const inf = 340282346638528859811704183484516925440.0;
      var ret = 
          (this.left <= -inf) &&
          (this.top <= -inf) &&
          (this.right >= inf) &&
          (this.bottom >= inf);
      return ret;
    }
  };

  Rect.fromLTRB = function(left, top, right, bottom) {
    var rect = new Rect();
    rect.left = left;
    rect.top = top;
    rect.right = right;
    rect.bottom = bottom;
    return rect;
  }

  Rect.fromNodeBounds = function(node) {
    return Rect.fromLTRB(
        Number(node.properties.ActualRect.left),
        Number(node.properties.ActualRect.top),
        Number(node.properties.ActualRect.left + node.properties.ActualRect.width),
        Number(node.properties.ActualRect.top + node.properties.ActualRect.height));
        //Number(node.properties.boundsLeft),
        //Number(node.properties.boundsTop),
        //Number(node.properties.boundsRight),
        //Number(node.properties.boundsBottom));
  }

  Rect.fromMILRect = function(milRect) {
    return Rect.fromLTRB(
        Number(milRect.left),
        Number(milRect.top),
        Number(milRect.right),
        Number(milRect.bottom));
  }


  ///////////////////////////////////////////////////////////
  //
  // UI
  //
  ///////////////////////////////////////////////////////////

  var UI = function() {
  };

  UI.prototype = {
    _createElement: function(type) {
      var element = document.createElement(type);
      element.ownerUI = this;
      return element;
    },
    // Add a css class name to an element (IE doesn't support classList)
    _addClassName: function(element, name) {
      $(element).addClass(name);
    },
    // Remove a css class name to an element (IE doesn't support classList)
    _removeClassName: function(element, name) {
      $(element).removeClass(name);
    },
    // Adds or removes class name based on val
    _setClassName: function(element, name, val) {
      if (val) {
        this._addClassName(element, name);
      }
      else {
        this._removeClassName(element, name);
      }
    },

    addClassName: function(className) {
      this._addClassName(this._container, className);
    },
    removeClassName: function(className) {
      this._removeClassName(this._container, className);
    },
    setClassName: function(className, value) {
      this._setClassName(this._container, className, value);
    }
  };

  var extend = function(base, constructor, members) {
    var newPrototype = Object.create(base.prototype);

    var keys = Object.keys(members);
    for (var i = 0; i < keys.length; i++) {
      var prop = Object.getOwnPropertyDescriptor(members, keys[i]);
      Object.defineProperty(newPrototype, keys[i], prop);
    }

    constructor.prototype = newPrototype;
    return constructor;
  };


  ///////////////////////////////////////////////////////////
  //
  // TreeViewItem
  //
  ///////////////////////////////////////////////////////////

  var TreeViewItem = extend(UI, function(node) {
    this._node = node;
    this._container = this._createElement("treeviewitem");
    node._treeViewItem = this;

    // create the expando element and add a click handler
    this._expando = this._createElement("expando");
    var me = this;
    this._expando.onclick = function(e){
      me.expanded = !me.expanded;
    };

    this._container.appendChild(this._expando);

    // create the text label
    this._label = this._createElement("span");

    this._label.ondblclick = function(e){
      me.expanded = !me.expanded;
    };

    var itemText = node.name;
    if (isDefined(node._notes)) {
      itemText += ' (';
      for (const i in node._notes) {
        if (i != 0) {
          itemText += ', ';
        }

        itemText += node._notes[i];
      }
      itemText += ')';
    }

    this.text = itemText;

    this._container.appendChild(this._label);

    this._containsWarnings = isDefined(node._warnings);
    if (this._containsWarnings) {
      for (const warning of node._warnings) {
        var issue = this._createElement("div");
        $(issue).addClass('warning');
        issue.innerHTML = warning;
        this._container.appendChild(issue);
      }
    }

    this._cyclic = false;
    this.setClassName("isvisual", this._node._isVisual);

    this._expanded = this._node._isVisual;

    if (this._node._isVisual) {
      this.setClassName("hasbounds", this._node._hasNonEmptyBounds);
    }
  },
  // Members
  {
    // Sets css class names on the elements
    _updateClassNames: function() {
      this.setClassName("empty", !this._hasChildren);
      this.setClassName("expanded", this._hasChildren && this.expanded);
      this.setClassName("collapsed", this._hasChildren && !this.expanded);
      this.setClassName("cyclic", this.cyclic);
      this.setClassName("hascontent", this._hasContent);
      this.setClassName("lookatme", this._containsWarnings);
    },

    set onclick(value) {
      this._label.onclick = value;
    },

    set onmouseover(value) {
      this._label.onmouseover = value;
    },

    set onmouseout(value) {
      this._label.onmouseout = value;
    },

    // expanded
    get expanded() {
      return this._expanded;
    },
    set expanded(value) {
      if (this._expanded === undefined || this._expanded != value) {
        this._expanded = value;
        this._updateClassNames();

        markSubtreeCollapsed(this._node, !value);
      }
    },

    // text - the text displayed in the treeviewitem
    get text() {
      return this._label.innerHTML;
    },
    set text(value) {
      this._label.innerHTML = value;
    },

    get cyclic() {
      return this._cyclic;
    },

    set cyclic(value) {
      if (this._cyclic != value) {
        this._cyclic = value;
        this._updateClassNames();
      }
    },

    get isVisual() {
      return this._node._isVisual;
    },

    set hasContent(value) {
      // add should always follow this, so we don't need to update class names.
      this._hasContent = value;
    },

    // add - adds a child tree view item
    add: function(treeviewitem) {
      this._container.appendChild(treeviewitem._container);
      this._hasChildren = true;

      if (treeviewitem._containsWarnings) {
        // Don't propagate the warning style out of visuals used as resources
        if (this._node._isVisual || !treeviewitem._node._isVisual) {
          this._containsWarnings = true;
        }
      }

      this._updateClassNames();

      if (this._node._isVisual && treeviewitem._node._isVisual && 
          (!arraysAreSame(this._node.channels, treeviewitem._node.channels))) {

        var processIds = {};

        for (const channelHandle of treeviewitem._node.channels) {
          var channel = channels[channelHandle];
          if (!isDefined(processIds[channel.pid])) {
            processIds[channel.pid] = [];
          }

          processIds[channel.pid].push(channelHandle);
        }

        var details = treeviewitem._createElement("div");

        var detailsText = '';
        for (const pid in processIds) {
          var channelHandles = processIds[pid];
          var processText = processNames[pid] === '' ? 'Process ' + pid : processNames[pid] + ' (' + pid + ')';
          var channelText  = channelHandles.length === 1 ? ' on channel ' : ' on channels ';
          detailsText += '<div class="processdetail">' + processText + channelText + channelHandles + '</div>';
        }

        details.innerHTML = detailsText;

        treeviewitem.addClassName('processblock');
        treeviewitem._container.insertAdjacentElement('afterbegin', details);
      }
    },
  });


  ///////////////////////////////////////////////////////////
  //
  // Splitter
  //
  ///////////////////////////////////////////////////////////

  var Splitter = extend(UI, function(pane1, pane2, options) {
    options = options || {};
    this._container = document.createElement("splitter");

    this._vertical = options.vertical;
    this.addClassName(this._vertical ? "vertical" : "horizontal");

    this._gripper = document.createElement("gripper");

    this._pane1 = pane1;
    this._addClassName(pane1, "pane1");

    this._pane2 = pane2;
    this._addClassName(pane2, "pane2");

    this._container.appendChild(pane1);
    this._container.appendChild(this._gripper);
    this._container.appendChild(pane2);

    var startMousePoint = 0, startingPosition;
    this._gripper.addEventListener("mousedown", function(e) {
      if (e.button == 0) {
        document.addEventListener("mouseup", mouseup, true);
        document.addEventListener("mousemove", mousemove, true);

        startMousePoint = this._vertical ? e.screenX : e.screenY;
        startingPosition = this._position;

        e.preventDefault();
        return false;
      }
    }.bind(this), false);

    var mouseup = function(e) {
      if (e.button == 0) {
        document.removeEventListener("mouseup", mouseup, true);
        document.removeEventListener("mousemove", mousemove, true);

        return false;
      }
    }.bind(this);

    var mousemove = function(e) {
      var newPoint = this._vertical ? e.screenX : e.screenY;
      var delta = newPoint - startMousePoint;

      if (!this._vertical) {
        delta *= -1;
      }
      this._setPosition(delta + startingPosition);

      return false;
    }.bind(this);

    var defaultSize = this._vertical ? window.innerWidth / 2 : window.innerHeight / 2;

    this._setPosition(options.position || defaultSize);
  },
  {
    _setPosition: function(position) {
      if (this._position != position) {
        this._position = position = Math.max(10, position)
        if (this._vertical) {
          this._pane1.style.width = position + 'px';
          this._gripper.style.left = position + 'px';
          this._pane2.style.left = (position + 6) + 'px';
        }
        else {
          this._pane1.style.bottom = (position + 6) + 'px';
          this._gripper.style.bottom = position + 'px';
          this._pane2.style.height = position + 'px';
        }
        if (this.onresize){
          this.onresize();
        }
      }
    }
  });


  ///////////////////////////////////////////////////////////
  //
  // loadData
  //
  ///////////////////////////////////////////////////////////

  var nodeTree;
  var channels;
  var processNames;

  var buildDataTree = function()  {
    var rawData;
    try
    {
        rawData = JSON.parse(document.getElementById("json_data2").innerText);
    }
    catch (e)
    {
        alert("JSON parse exception: " + e );
        throw e;
    }

    // Build lookup dict
    var lookupDict = {};
    for (const node of rawData.nodes) {
      lookupDict[node.address] = node;
    }

    for (const node of rawData.nodes) {
      if (isDefined(node.children))
      {
        var newChildren = $.map(node.children, function(childAddress) {
          return lookupDict[childAddress];
        });

        node.children = newChildren;
        node._isVisual = true;
      }

      if (isDefined(node.links)) {
        // Map the link property names to nodes and build a deduplicated
        // array of those nodes, like the children.
        node.resources = [];
        for (const key in node.links) {
          const address = node.links[key];
          const resource = lookupDict[address];  // maybe null
          node.links[key] = resource;
          if (resource && !node.resources.includes(resource)) {
            node.resources.push(resource);
          }
        }
      }
    }


    nodeTree = rawData.nodes[0];
    nodeTree.channels = [];
  }

  var loadData = function() {
    buildDataTree();

    var init = new NodeInitializerVisitor();
    init._node = nodeTree;
    walkTree(init);

    var roundedRects = new RoundedRectVistor();
    roundedRects._node = nodeTree;
    walkTree(roundedRects);

    var opacityLayers = new LayerOpacityVisitor();
    opacityLayers._node = nodeTree;
    walkTree(opacityLayers);

    var infiniteBounds = new InfiniteBoundsVisitor();
    infiniteBounds._node = nodeTree;
    walkTree(infiniteBounds);

    var complexEffect = new ComplexEffectVisitor();
    complexEffect._node = nodeTree;
    walkTree(complexEffect);

    var hollowNinegrid = new HollowNinegridVisitor();
    hollowNinegrid._node = nodeTree;
    walkTree(hollowNinegrid);
  };

  // Highlights nodes based on filter criteria
  // and collapses irrelevant subtrees
  var filterTree = function() {
    var filterVisitor = new FilterVisitor();
    filterVisitor._node = nodeTree;
    walkTree(filterVisitor);
    updateCanvas();
  }

  // Expands all expandos
  var expandAll = function() {
    var expandAllVisitor = new ExpandAllVisitor();
    expandAllVisitor._node = nodeTree;
    walkTree(expandAllVisitor);
    updateCanvas();
  }


  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////
  //
  // App Stuff
  //
  ///////////////////////////////////////////////////////////
  ///////////////////////////////////////////////////////////

  var canvas;
  window.onload = function() {
    loadData();

    const buildCtxt = {};
    buildCtxt.node = nodeTree
    var unifiedTreeViewItem = buildUnifiedTreeView(buildCtxt);

    var tree = document.createElement("div");
    tree.id = "tree";

    var filter = document.createElement("div");
    filter.id = "filter"
    
    tree.appendChild(filter);
    tree.appendChild(unifiedTreeViewItem._container);

    var nodeNameFilterInput = document.createElement("input");
    nodeNameFilterInput.type = "text";
    nodeNameFilterInput.id = "nodeNameFilterInput";
    nodeNameFilterInput.placeholder = "Filter by Node Name";
    nodeNameFilterInput.oninput = filterTree;
    filter.appendChild(nodeNameFilterInput);

    filter.appendChild(document.createElement('br'));

    var propertyNameFilterInput = document.createElement("input");
    propertyNameFilterInput.type = "text";
    propertyNameFilterInput.id = "propertyNameFilterInput";
    propertyNameFilterInput.placeholder ="Filter by Property Name";
    propertyNameFilterInput.oninput = filterTree;
    filter.appendChild(propertyNameFilterInput);

    var addOption = function(filterTypeSelect, type)
    {
      var option = document.createElement('option')
      option.value = type;
      option.innerHTML = type;
      filterTypeSelect.appendChild(option);
    }

    var filterTypeSelect = document.createElement('select');
    filterTypeSelect.id = 'compareTypeSelect';
    filter.appendChild(filterTypeSelect);
    filterTypeSelect.onchange = filterTree;

    addOption(filterTypeSelect, "contains");
    addOption(filterTypeSelect, "==");
    addOption(filterTypeSelect, "<");
    addOption(filterTypeSelect, ">");
    addOption(filterTypeSelect, "<=");
    addOption(filterTypeSelect, ">=");
    addOption(filterTypeSelect, "!=");

    var propertyValueFilterInput = document.createElement("input");
    propertyValueFilterInput.type = "text";
    propertyValueFilterInput.id = "propertyValueFilterInput";
    propertyValueFilterInput.placeholder ="Filter by Address or Value";
    propertyValueFilterInput.oninput = filterTree;
    filter.appendChild(propertyValueFilterInput);

    // Filters by (Name OR (PropertyName AND PropertyValue))
    
    filter.appendChild(document.createElement('br'));
    var expandAllButton = document.createElement("button");
    expandAllButton.innerHTML = "Expand All"
    expandAllButton.onclick = expandAll;
    filter.appendChild(expandAllButton);

    var canvas = document.createElement("canvas");
    canvas.id = "canvas";
    canvas.setAttribute('tabindex', '0');   // Enable keyboard input.

    var canvasHolder = document.createElement("div");
    canvasHolder.id = "canvasHolder";

    canvasHolder.appendChild(canvas);

    var details = document.createElement("div");
    details.id = "details";

    var horizontalSplitter = new Splitter(canvasHolder, details, {position: 400});
    horizontalSplitter.onresize = updateCanvas;

    var verticalSplitter = new Splitter(tree, horizontalSplitter._container, {vertical: true, position: window.innerWidth / 2});
    verticalSplitter.onresize = updateCanvas;

    document.body.appendChild(verticalSplitter._container);

    updateCanvas();
  };

  var walkTree = function(context) {
    var node = context._node;

    if (node._visited) {
      return; // Prevent cycles
    }

    node._visited = true;
    context.PreSubgraph();

    // The visitor will null out the node if doesn't want to go deeper
    if (context._node !== null) {
      if (!context._excludeLinks && isDefined(node.resources)) {
        for (const linkedChild of node.resources) {
          // Do not (re)visit visuals that are a property of some resource (for example,
          // VisualSurface->[VisualTree]->Visual).  All walks assume visuals are unique
          // with the exception of populating the treeview.
          if (!linkedChild._isVisual) {
            var childContext = context.Clone();
            childContext._node = linkedChild;
            walkTree(childContext);
          }
        }
      }

      if (!context._excludeChildren && isDefined(node.children)) {
        for (const child of node.children) {
          var childContext = context.Clone();
          childContext._node = child;
          walkTree(childContext);
        }
      }
    }

    context._node = node;
    context.PostSubgraph();
    node._visited = false;
  }

  var numBitmapsLoaded = 0;

  var NodeInitializerVisitor = function() {
    this._currentVisual = null;
  }

  NodeInitializerVisitor.prototype = {
    Clone : function() {
      var clone = new NodeInitializerVisitor();
      clone._parentContext = this;
      clone._currentVisual = this._currentVisual;
      return clone;
    },

    PreSubgraph : function() {
      if (this._node._isVisual) {
        this._currentVisual = this._node;

        // If parent has 0 opacity, its bounds are empty, but bounds are still
        // bookkept for its descendants though they will not ever render.  We will
        // propagate empty bounds down the subtree to handle that, and the visible bit being unset.
        this._node._bounds = Rect.fromNodeBounds(this._node);

        var isInvisible = isDefined(this._node.properties) && (this._node.properties.m_fVisibility == 1);

        const parentHasNonEmptyBounds = isDefined(this._parentContext) ?
          this._parentContext._node._hasNonEmptyBounds : true;
        this._node._hasNonEmptyBounds = parentHasNonEmptyBounds && !isInvisible && !this._node._bounds.isEmpty();

        // Convert to global bounds
        if (this._node._hasNonEmptyBounds && isDefined(this._parentContext)) {
            this._node._bounds.left  += this._parentContext._node._bounds.left;
            this._node._bounds.right += this._parentContext._node._bounds.left;
            this._node._bounds.top    += this._parentContext._node._bounds.top;
            this._node._bounds.bottom += this._parentContext._node._bounds.top;
        }

        // Window node needs special bounds handling: Bitmaps are stretched to the window rect but clipped
        // to bounds. This is perfect for sprite bitmaps, flipex bitmaps should stretch to client rect.
        if (this._node._hasNonEmptyBounds && 
          (this._node.name === 'CWindowNode') &&
          isDefined(this._node.properties) &&
          (isDefined(this._node.properties.m_contentRelativeWindowRect))) {
          var windowRect = Rect.fromMILRect(this._node.properties.m_contentRelativeWindowRect);
          this._node._worldTransform = Matrix4x4.fromVisualTreeData(this._node);
          this._node._bitmapDstRect = mapLocalBoundsToWorld(windowRect, this._node);

          var clip = new Path2D;
          clip.rect(this._node._bounds.left, this._node._bounds.top, this._node._bounds.width(), this._node._bounds.height());
          this._node._clipPath = clip;
        }

        // Define the initial ancestor collapse on each visual.  This
        // will be undefined and evaluate to false on resources.
        this._node._viewCollapsedOnAncestor = false;

        // Sort the channels array for easy comparison
        if (!isDefined(this._node.channels)) {
          this._node.channels = [];
        }

        this._node.channels.sort((a, b) => a - b);
      }
      else {
        // Not a visual
        if (!isDefined(this._node._referencingVisuals)) {
          this._node._referencingVisuals = [];
        }

        // Some visuals might have multiple references to the same resource,
        // Ex: primitive groups, compound brushes.
        if (!this._node._referencingVisuals.includes(this._currentVisual)) {
          this._node._referencingVisuals.push(this._currentVisual);
        }

      }

      if (isDefined(this._node.properties)) {
        var properties = this._node.properties;

        // Preprocess bitmap source rects
        if (isDefined(properties.m_rcfSurfaceContentBounds)) {
          this._node._sourceRect = Rect.fromLTRB(
              Number(properties.m_rcfSurfaceContentBounds.left),
              Number(properties.m_rcfSurfaceContentBounds.top),
              Number(properties.m_rcfSurfaceContentBounds.right),
              Number(properties.m_rcfSurfaceContentBounds.bottom));
        }
        else if (
            isDefined(properties.SurfaceRectLeft) && 
            isDefined(properties.SurfaceRectRight) &&
            isDefined(properties.SurfaceRectTop) && 
            isDefined(properties.SurfaceRectBottom)) {
          this._node._sourceRect = Rect.fromLTRB(
              Number(properties.SurfaceRectLeft),
              Number(properties.SurfaceRectTop),
              Number(properties.SurfaceRectRight),
              Number(properties.SurfaceRectBottom));
        }

        // Find and register any bitmaps
        if (isDefined(properties.imageFile)) {
          this._node._bitmapProxy = getProxyForImage(properties.imageFile);
        }

        // Add notes visible in the treeview
        if (isDefined(properties.m_flags)) {
          if (properties.m_flags.WeaklyReferenced) {
            addNodeNote(this._node, 'AnimationTarget');
          }
        }

        if (isDefined(properties.m_sparseStorage)) {
          if (isDefined(properties.m_sparseStorage.OpacityInternalId)) {
            addNodeNote(this._node, 'Opacity');
          }
        }

        if (this._node.name === 'CColorBrush') {
          if (isDefined(properties.m_color)) {
            if (Number(properties.m_color.a) === 0) {
              // Check if we're inside a compound brush
              var parentContext = this._parentContext;
              for (;;) {
                if (parentContext._node == this._currentVisual) {
                  addNodeNote(this._currentVisual, 'Hittest Only');
                  break;
                }

                if ((parentContext._node.name !== 'CNineGridBrush') &&
                    (parentContext._node.name !== 'CMaskBrush')) {
                  break;
                }

                parentContext = parentContext._parentContext;
              }
            }
          }
        }

        if (this._node.name === 'CLayerVisual') {
          var isActive = false;

          var effectVar = properties.m_pTreeEffect;
          if (!isDefined(effectVar)) {
            // Older variant
            effectVar = properties.m_pTreeEffectNoRef;
          }

          const treeEffect = parseInt(effectVar, 16);
          if (!isNaN(treeEffect) && (treeEffect != 0)) {
            isActive = true;
          }

          if (isDefined(this._node.links) && this._node.links.m_pShadow) {
            isActive = true;
          }

          if (isActive) {
            addNodeWarning(this._node, 'Active layer visual', 'LayerVisual');
          }
        }
        else if (this._node.name.endsWith('Effect')) {
          // Filter effect on visual
          if (this._parentContext && (this._currentVisual == this._parentContext._node)) {
            addNodeWarning(this._currentVisual, 'Filter effect', 'LayerEffect');
          }
        }

        if (isDefined(properties.m_sparsePointerStorage)) {
          for (var prop in properties.m_sparsePointerStorage) {
            if (prop.startsWith('ProjectedShadowReceiversId')) {
              addNodeNote(this._currentVisual, 'Projected Shadow');
            }
            else if (prop === 'SpriteVisual_DropShadowId') {
              addNodeNote(this._currentVisual, 'Drop Shadow');
            }
            else if (prop === 'InteractionInternalId') {
              addNodeNote(this._currentVisual, 'Interaction');
            }
          }
        }
      }
    },

    PostSubgraph : function() {
    }
  };


  var RoundedRectVistor = function() {
    this._excludeLinks = true;
  }

  RoundedRectVistor.prototype = {
    Clone : function() {
      var clone = new RoundedRectVistor();
      clone._parentContext = this;
      return clone;
    },

    PreSubgraph : function() {
      // Check the clip property on the visual
      if (isDefined(this._node.links) && isDefined(this._node.links.m_pClip)) {
        var clip = this._node.links.m_pClip;
        if ((clip !== null) && (clip.name === 'CRectangleGeometry')) {
          var clipData = clip.properties.m_data;

          // Corner radius is defined either as 8 floats or 4 D2D1_VECTOR_2F structs. Check for both formats.
          var noRoundedCorner = true;
          if (clipData['m_TopLeftRadius'] !== undefined) {
            var newCornerProps = ['m_TopLeftRadius', 'm_TopRightRadius', 'm_BottomLeftRadius', 'm_BottomRightRadius'];
            noRoundedCorner = newCornerProps.every((prop) => (clipData[prop].x === 0 && clipData[prop].y === 0));
          }
          else {
            var oldCornerProps = ['m_TopLeftRadiusX', 'm_TopLeftRadiusY', 'm_TopRightRadiusX', 'm_TopRightRadiusY',
              'm_BottomLeftRadiusX', 'm_BottomLeftRadiusY', 'm_BottomRightRadiusX', 'm_BottomRightRadiusY'];
            noRoundedCorner = oldCornerProps.every((prop) => (clipData[prop] === 0));
          }

          if (!noRoundedCorner) {
            addNodeWarning(this._node, 'Rounded corner clip', 'RoundedClip');
          }
        }
      }
    },

    PostSubgraph : function() {
    }
  };


  var LayerOpacityVisitor = function() {
    this._excludeLinks = true;
    this._usingPushDownOpacity = false;
  }

  LayerOpacityVisitor.prototype = {
    Clone : function() {
      var clone = new LayerOpacityVisitor();
      clone._usingPushDownOpacity = this._usingPushDownOpacity;
      return clone;
    },

    PreSubgraph : function() {
      if (!isDefined(this._node.properties)) {
        return;
      }

      // Update the context if not already doing push-down
      if (!this._usingPushDownOpacity) {
        // Todo, check renderoptions flags if opacity mode is valid
        if (this._node.properties.m_opacityMode === 1) {
          this._usingPushDownOpacity = true;
        }
      }

      // Check for push down opacity
      if (this._usingPushDownOpacity) {
        return;
      }

      // Does it have descendants and is it not using 'Spatial' depth mode?
      if (isDefined(this._node.children) && (this._node.children.length !== 0) &&
          (this._node.properties.m_effectiveDepthMode !== 'Spatial')) {
        // Check if the visual has non-1 opacity
        var layerOpacity = false;
        if (isDefined(this._node.properties.m_sparseStorage) &&
            isDefined(this._node.properties.m_sparseStorage.OpacityInternalId)) {
          layerOpacity = true;
        }
        // Check for an opacity effect
        if (isDefined(this._node.resources)) {
          for (const link of this._node.resources) {
            if (link.name === 'CEffectGroup') {
              if ((link.properties.m_opacity != 0) && (link.properties.m_opacity != 1)) {
                layerOpacity = true;
              }
              break;
            }
          }
        }

        if (layerOpacity) {
          addNodeWarning(this._node, 'Layer Opacity', 'LayerOpacity');
        }
      }
    },

    PostSubgraph : function() {
    }
  };

  // Helpers for FilterVisitor

  var requiresFloatParsing = function(compareType) {
    return compareType === "<" || compareType === ">" || compareType === "<=" || compareType === ">=";
  }

  // Value1 is passed in directly from the property of the Visual
  // Value2 is what the user typed into the filter
  var getComparator = function(compareType) {
    if (compareType === "==") {
      return function(value1, value2) {
        value1Float = parseFloat(value1);
        value2Float = parseFloat(value2);
        if (!isNaN(value1Float) && !isNaN(value2Float)) {
          return value1Float === value2Float;
        }
        return ("" + value1).toLowerCase() === value2;
      }
    } else if (compareType === "contains") {
      return function(value1, value2) {
        return ("" + value1).toLowerCase().includes(value2);
      }
    } else if (compareType === "!=") {
      return function(value1, value2) {
        value1Float = parseFloat(value1);
        value2Float = parseFloat(value2);
        if (!isNaN(value1Float) && !isNaN(value2Float)) {
          return value1Float != value2Float;
        }
        return value1 != value2;
      }
    } else if (compareType === "<") {
      return function(value1, value2) {
        value1 = parseFloat(value1);
        if (isNaN(value1)) {
          return false;
        }
        return value1 < value2;
      }
    } else if (compareType === ">") {
      return function(value1, value2) {
        value1 = parseFloat(value1);
        if (isNaN(value1)) {
          return false;
        }
        return value1 > value2;
      }
    } else if (compareType === "<=") {
      return function(value1, value2) {
        value1 = parseFloat(value1);
        if (isNaN(value1)) {
          return false;
        }
        return value1 <= value2;
      }
    } else if (compareType === ">=") {
      return function(value1, value2) {
        value1 = parseFloat(value1);
        if (isNaN(value1)) {
          return false;
        }
        return value1 >= value2;
      }
    } 
  }

  function convertX64Value(s) {
      if (s.indexOf('\`') >= 0)
      {
          var parts = s.split('\`');
          return parts[0] + parts[1];
      }
      return s;
  }

  // Based on filter criteria inputs, highlights relevant nodes and collapses irrelevant subtrees
  var FilterVisitor = function() {
  }

  FilterVisitor.prototype = {
    Clone : function() {
      var clone = new FilterVisitor();
      clone._parentContext = this;
      return clone;
    },

    // Assumes either property filter or value filter is not empty string
    // property: object containing property data
    // propertyNameSplit: name of the property split by "." e.g. ['offset', 'x']
    // propertyValue: the value we are filtering by
    // maxDepth: the max depth to recuse.  This is used to limit searching through links.
    FindProperty : function(property, propertyNameSplit, propertyValue, comparator, maxDepth) {
      if (maxDepth === 0) {
        return false;
      }
      if (typeof(property) === 'object') {
        var isPropertyNameEmpty = propertyNameSplit[0] === '';
        var propertyName = propertyNameSplit[0];

        for(var field in property) {
          // If we are not at the final segment of the property (e.g. 'offset' in 'offset.x')
          if (!isPropertyNameEmpty && propertyNameSplit.length > 1) {
            if (field.toLowerCase() === propertyName) {
              // remove the first property name and recurse
              if (this.FindProperty(property[field], propertyNameSplit.slice(1), propertyValue, comparator, maxDepth-1)) {
                return true;
              }
            }
          }
          else {
            // Check to see if the property name and property values match
            if (isPropertyNameEmpty || field.toLowerCase().includes(propertyName)) {
              var value = property[field];
              if (propertyValue === '') {
                // Nothing was entered into the value filter
                return true;
              } else if (comparator(value, propertyValue)) {
                return true;
              }
            }
          }
          if (this.FindProperty(property[field], propertyNameSplit, propertyValue, comparator, maxDepth-1)) {
            return true;
          }
        }
      }
      return false;
    },

    PreSubgraph : function() {
      this._node._hasMark = false;
      this._node._hasMarkInSubgraph = false;

      var nodeNameFilter = document.getElementById('nodeNameFilterInput').value;
      var propertyNameFilter = document.getElementById('propertyNameFilterInput').value;
      var propertyValueFilter = convertX64Value(document.getElementById('propertyValueFilterInput').value);
      var compareType = document.getElementById('compareTypeSelect').value;
      
      // Reset marking
      var newNodeHTML = this._node._treeViewItem.text;
      newNodeHTML = newNodeHTML.replace('<mark>', '');
      newNodeHTML = newNodeHTML.replace('</mark>', '');
      newNodeHTML = newNodeHTML.replace('<mark style="background-color:lightgreen;">', '');
      newNodeHTML = newNodeHTML.replace('</mark>', '');
      
      var substringIndex = newNodeHTML.toLowerCase().indexOf(nodeNameFilter.toLowerCase());
      if (substringIndex > -1 && nodeNameFilter != '') {
        // Mark the substring of the name yellow
        var substring = newNodeHTML.substring(substringIndex, substringIndex + nodeNameFilter.length);
        newNodeHTML = newNodeHTML.replace(substring, '<mark>' + substring + '</mark>')
        this._node._hasMark = true;
      }

      // Search properties
      if (propertyNameFilter != '' || propertyValueFilter != '') {
        // Parse property name
        propertyNameSplit = propertyNameFilter.toLowerCase().split('.');
        var isPropertyNameEmpty = propertyNameSplit.length === 1 && propertyNameSplit[0] === '';

        var comparator = getComparator(compareType);
        if (propertyValueFilter != '') {
          if (requiresFloatParsing(compareType)) {
            propertyValueFilter = parseFloat(propertyValueFilter);
            if (isNaN(propertyValueFilter)) {
              // console.log("ERROR: Input needs to be a number");
            }
          }
          else {
            propertyValueFilter = propertyValueFilter.toLowerCase();
          }
        }

        // Search the node's properties and links
        var foundProperty =
          this.FindProperty(this._node.properties, propertyNameSplit, propertyValueFilter, comparator, -1) ||
          this.FindProperty(this._node.links, propertyNameSplit, propertyValueFilter, comparator, 2) || foundProperty;

        if (!foundProperty && !requiresFloatParsing(compareType)) {
          // Search the node's address
          foundProperty =
            (propertyNameFilter.toLowerCase() === "address" || propertyNameFilter.toLowerCase() === '') &&
              propertyValueFilter.toLowerCase() === this._node.address;
        }

        if (foundProperty) {
          // Mark the node green if we found a property match
          newNodeHTML = '<mark style="background-color:lightgreen;">' + newNodeHTML + '</mark>';
          this._node._hasMark = true;
        }
      }

      this._node._treeViewItem.text = newNodeHTML;
    },

    PostSubgraph : function() {
      var nodeNameFilter = document.getElementById('nodeNameFilterInput').value;
      var propertyNameFilter = document.getElementById('propertyNameFilterInput').value;
      var propertyValueFilter = convertX64Value(document.getElementById('propertyValueFilterInput').value);

      if (nodeNameFilter === '' && propertyNameFilter === '' && propertyValueFilter === '') {
        // If we are filtering by nothing, unexpand the entire tree
        this._node._treeViewItem.expanded = true;
        return;
      }

      // Propagate hasMarkInSubgraph so we know what to keep expanded
      // Add the mark notation on the node since contexts do not keep track of their children,
      // and the parent will need to know which children are marked to appropriately collapse them.
      if (this._node._hasMark) {
        this._node._hasMarkInSubgraph = true;
      }
      if (this._node._hasMarkInSubgraph) {
        if (isDefined(this._parentContext)) {
          this._parentContext._node._hasMarkInSubgraph = true;
        }
      }

      // Find children who need to be collapsed
      for (var i = 0; i < this._node.children.length; i++) {
        var child = this._node.children[i];
        // Only collapse a child if it is unmarked but the parent is marked.
        // That way, the entire subtree can be expanded with a single click.
        if (!child._hasMarkInSubgraph && this._node._hasMarkInSubgraph) {
          child._treeViewItem.expanded = false;
        }
        else {
          child._treeViewItem.expanded = true;
        }
      }

      // Root case
      if (!isDefined(this._parentContext)) {
        this._node._treeViewItem.expanded = this._node._hasMarkInSubgraph;
      }
    }
  };

  // Expands all expandos in the tree
  var ExpandAllVisitor = function() {
  }

  ExpandAllVisitor.prototype = {
    Clone : function() {
      var clone = new ExpandAllVisitor();
      clone._parentContext = this;
      return clone;
    },

    PreSubgraph : function() {},

    PostSubgraph : function() {
      this._node._treeViewItem.expanded = true;
    }
  };

  var InfiniteBoundsVisitor = function() {
    this._excludeLinks = true;
  }

  InfiniteBoundsVisitor.prototype = {
    Clone : function() {
      var clone = new InfiniteBoundsVisitor();
      clone._parentContext = this;
      return clone;
    },

    PreSubgraph : function() {
      this._hasInfiniteBounds = this._node._bounds.isInfinite();
      this._hasInfiniteBoundsInSubgraph = false;

      if (!this._hasInfiniteBounds) {
        // We don't need to go into this subtree
        this._node = null;
        return;
      }

      if (isDefined(this._node.links) && (this._node.links.m_pClip != null)) {
        // A clip will "fix" infinite bounds from descendants, but this isn't
        // reflected in node bounds - infinity will go all the way to the root.
        this._hasInfiniteBoundsInSubgraph = true;
        this._node = null;
        return;
      }
    },

    PostSubgraph : function() {
      if (this._hasInfiniteBounds) {
        if (isDefined(this._parentContext)) {
          this._parentContext._hasInfiniteBoundsInSubgraph = true;
        }

        if (!this._hasInfiniteBoundsInSubgraph) {
          // Something about this node is making its bounds infinite
          if (isDefined(this._node.properties.m_sparsePointerStorage) &&
            isDefined(this._node.properties.m_sparsePointerStorage.TransformParentDataInternalId)) {

            addNodeWarning(this._node, 'Infinite bounds (transform parent)', 'InfiniteBounds');
          }
          else {
            addNodeWarning(this._node, 'Infinite bounds', 'InfiniteBounds');
          }
        }
      }
    }
  };


  CollapsedViewVisitor = function() {
    this._excludeLinks = true;
  }

  CollapsedViewVisitor.prototype = {
    Clone : function() {
      var isCollapsed = this._collapsedAncestor || this._node._viewCollapsed;
      var clone = new CollapsedViewVisitor();
      clone._collapsedAncestor = isCollapsed;
      return clone;
    },

    PreSubgraph : function() {
      if (!this._isRoot &&
          (this._node._viewCollapsedOnAncestor === this._collapsedAncestor)) {
        // We don't need to go into this subtree
        this._node = null;
        return;
      }

      this._node._viewCollapsedOnAncestor = this._collapsedAncestor;
    },

    PostSubgraph : function() {
    }
  };


  var ComplexEffectVisitor = function() {
    this._currentVisual = null;
  }

  ComplexEffectVisitor.prototype = {
    Clone : function() {
      var clone = new ComplexEffectVisitor();
      clone._parentContext = this;
      clone._currentVisual = this._currentVisual;
      return clone;
    },

    PreSubgraph : function() {
      if (this._node._isVisual) {
        this._currentVisual = this._node;
        return;
      }

      if (this._node.name === 'FlattenedEffectGraph') {
        var properties = this._node.properties;
        var subgraphCount = 0;
        var nodeCount = 0;
        var borderCount = 0;
        for (const prop in properties) {
          if (prop.startsWith('m_subgraphs')) {
            subgraphCount += 1;
          }
          else if (prop.startsWith('m_nodes')) {
            nodeCount += 1;
          }
          else if (prop.startsWith('m_namedInputs')) {
            if (properties[prop].fHasBorderEffect) {
              borderCount += 1
            }
          }
        }

        if ((subgraphCount > 1) || (borderCount > 0)) {
          addNodeWarning(this._currentVisual, 'Effect has ' + nodeCount + ' operations, '+ subgraphCount + ' stages and ' +
            borderCount + ' inputs with border', 'ExpensiveEffect');
        }
      }
      else if (this._node.name === 'CEffectBrush') {
        var properties = this._node.links;
        var backdropCount = 0;
        for (const prop in properties) {
          if (prop.startsWith('m_rgpInputs')) {
            if ((properties[prop].name === 'CBackdropBrush') ||
                (properties[prop].name === 'CWindowBackdropBrush')) {
              backdropCount += 1;
            }
          }
        }

        if (backdropCount > 0) {
          addNodeWarning(this._currentVisual, 'Effect has backdrop input', 'ExpensiveEffect');
        }
      }
    },

    PostSubgraph : function() {
    }
  };


  var HollowNinegridVisitor = function() {
    this._currentVisual = null;
  }

  HollowNinegridVisitor.prototype = {
    Clone : function() {
      var clone = new HollowNinegridVisitor();
      clone._parentContext = this;
      clone._currentVisual = this._currentVisual;
      return clone;
    },

    PreSubgraph : function() {
      if (this._node._isVisual) {
        this._currentVisual = this._node;

        if (!this._currentVisual._hasNonEmptyBounds) {
          this._node = null;
        }

        return;
      }
    },

    PostSubgraph : function() {
    }
  };

  var bitmapProxies = {};
  var getProxyForImage = function(imageName) {
    idVal = imageName.slice(0, -4); // remove '.bmp' from imageName

    var proxy = bitmapProxies[idVal];
    if (isDefined(proxy)) {
      return proxy;
    }

    proxy = { shouldDraw: false };
    bitmapProxies[idVal] = proxy;

    var image = document.getElementById(idVal);
    if (image) {
      // This image is embedded and already loaded
      numBitmapsLoaded++;
      proxy.image = image;
      proxy.shouldDraw = true;
      return proxy;
    }

    image = document.createElement("img");
    var imageContainer = document.getElementById("image_container");
      imageContainer.appendChild(image);

    proxy.image = image;

    image.addEventListener("load", function() {
      numBitmapsLoaded++;
      proxy.shouldDraw = true;
      updateCanvas();
    });
    image.src = imageName;
    image.id = idVal;

    return proxy;
  };

  var hasValidBitmapsToDraw = function() {
    for (const key in bitmapProxies) {
      if (bitmapProxies[key].shouldDraw) { 
        return true;
      }
    }
    return false;
  }

  var markSubtreeCollapsed = function(node, isCollapsed) {
    // Only the expanded state on treeview items that are visuals affect
    // the visualization.  We can't do it for resources since even though
    // they appear unique in the treeview, they are shared between multiple
    // visuals.
    if (node._isVisual)
    {
      node._viewCollapsed = isCollapsed;
      var collapseView = new CollapsedViewVisitor();

      // None of the node's ancestors can be collapsed, otherwise there
      // would be no way to make a change to this node's collapsed state.
      collapseView._node = node;
      collapseView._collapsedAncestor = node._viewCollapsedOnAncestor;
      collapseView._isRoot = true;

      walkTree(collapseView);
      updateCanvas();
    }
  }

  ///////////////////////////////////////////////////////////
  //
  // buildUnifiedTreeView
  //
  ///////////////////////////////////////////////////////////

  var buildUnifiedTreeView = function(buildCtxt) {
    const node = buildCtxt.node;
    var treeviewitem = new TreeViewItem(node);
    treeviewitem.onclick = function(){
      showDetails(node, treeviewitem);
      return false;
    };
    treeviewitem.onmouseover = function() {
      hoveredNode = node;
      updateCanvas();
    };
    treeviewitem.onmouseout = function() {
      hoveredNode = null;
      updateCanvas();
    };

    if (node._visited) {
      // We have found a cycle.
      // For now skip adding any children, and flag it as a cyclical reference
      treeviewitem.cyclic = true;
      return treeviewitem;
    }

    if (buildCtxt.isResource && node._isVisual) {
      // Do not recurse into visuals that are being used as resources
      return treeviewitem;
    }

    node._visited = true;
    var newBuildCtxt = {};
    newBuildCtxt.isResource = !node._isVisual;

    // First snag links
    if (isDefined(node.resources)) {
      if (node._isVisual && isDefined(node.links) && node.links.m_pContent) {
          treeviewitem.hasContent = true;
        }

      for (const linkedChild of node.resources) {
        newBuildCtxt.node = linkedChild;
        treeviewitem.add(buildUnifiedTreeView(newBuildCtxt));
      }
    }

    // Then get actual children
    if (isDefined(node.children)) {
      for (const child of node.children) {
        newBuildCtxt.node = child;
        treeviewitem.add(buildUnifiedTreeView(newBuildCtxt));
      }
    }

    node._visited = false;

    return treeviewitem;
  }

  ///////////////////////////////////////////////////////////
  //
  // showDetails
  //
  ///////////////////////////////////////////////////////////

  var selectedNode, hoveredNode, selectedTreeViewItem;
  var showDetails = function(node, treeviewitem) {
    if (selectedTreeViewItem) {
      selectedTreeViewItem.removeClassName("selected");
    }

    if (selectedTreeViewItem === treeviewitem) {
      // This node is already selected, now unselect it
      selectedTreeViewItem = null;
      selectedNode = null;
      updateCanvas();
      return;
    }

    selectedTreeViewItem = treeviewitem;
    selectedTreeViewItem.addClassName("selected");

    selectedNode = node;

    var details = document.getElementById("details");
    var detailContent = document.createElement("div");
    detailContent.innerHTML = '<div class="detailcontent">' + node.name + ' (' + node.address + ')</div>';

    var table = document.createElement("table");

    var tableHeader = document.createElement("tr");

    var headerElement2 = document.createElement("th");
    headerElement2.innerHTML = "Linked Properties";
    tableHeader.appendChild(headerElement2);

    var headerElement3 = document.createElement("th");
    headerElement3.innerHTML = "Other Properties";
    tableHeader.appendChild(headerElement3);

    table.appendChild(tableHeader);

    var addPropertyInfo = function(table, name, value, fontInfo){
      var tr = document.createElement("tr");

      var nametd = document.createElement("td");
      nametd.innerHTML = name;
      
      var filteredPropertyName = false;
      // Mark filtered properties
      var propertyNameFilter = document.getElementById('propertyNameFilterInput').value;
      if (propertyNameFilter != '') {
        propertyNameFilter = propertyNameFilter.split('.')[0];
        var substringIndex = name.toLowerCase().indexOf(propertyNameFilter.toLowerCase());
        if (substringIndex > -1)
        {
          // Mark the substring of the name yellow
          var substring = name.substring(substringIndex, substringIndex + propertyNameFilter.length);
          nametd.innerHTML = nametd.innerHTML.replace(substring, '<mark style="background-color:lightgreen;">' + substring + '</mark>')
          filteredPropertyName = true;
        }
      }

      var valuetd = document.createElement("td");
      if (fontInfo != undefined)
      {
        valuetd.style.fontFamily = fontInfo;
      }
      if (value.tagName == "A")
      {
        valuetd.appendChild(value);
      }
      else if (typeof(value) == 'object')
      {
        var innerTable = document.createElement("table");
        innerTable.className = "properties";

        for (var field in value) {
          addPropertyInfo(innerTable, field, value[field]);
        }
        valuetd.appendChild(innerTable);
      }
      else
      {
        valuetd.innerHTML = value;

        // Mark the value if it meets the filter
        if (value != null && (filteredPropertyName || propertyNameFilter == ''))
        {
          var valueString = value.toString();
          var propertyValueFilter = document.getElementById('propertyValueFilterInput').value;
          if (propertyValueFilter != '') {
            var substringIndex = valueString.toLowerCase().indexOf(propertyValueFilter.toLowerCase());
            if (substringIndex > -1)
            {
              // Mark the substring of the value
              var substring = valueString.substring(substringIndex, substringIndex + propertyValueFilter.length);
              valuetd.innerHTML = valuetd.innerHTML.replace(substring, '<mark style="background-color:lightgreen;">' + substring + '</mark>')
            }
          }
        }
      }

      tr.appendChild(nametd);
      tr.appendChild(valuetd);

      table.appendChild(tr);
    };

    var detailsRow = document.createElement("tr");

    var linkProperties = document.createElement("table");
    linkProperties.className = "properties";
    for (const key in node.links) {
      var linkedNode = node.links[key];
      var value;
      if (linkedNode) {
        value = linkedNode.name + " (" + linkedNode.address + ")";
      }
      else {
        value = "null";
      }
      addPropertyInfo(linkProperties, key, value);
    }
    var linkCell = document.createElement("td");
    linkCell.appendChild(linkProperties);
    detailsRow.appendChild(linkCell);

    var otherProperties = document.createElement("table");
    otherProperties.className = "properties";
    for (const key in node.properties) {
      var fontInfo = undefined;
      if (key == "Text") {
          fontInfo = node.properties["FontFamily"];
      }
      addPropertyInfo(otherProperties, key, node.properties[key], fontInfo);
    }
    var otherCell = document.createElement("td");
    otherCell.appendChild(otherProperties);
    detailsRow.appendChild(otherCell);

    table.appendChild(detailsRow);
    detailContent.appendChild(table);

    details.innerHTML = ""; // Clear previous set of details
    details.appendChild(detailContent);

    if (isDefined(node.properties.imageFile)) {
        var bitmapProxy = getProxyForImage(node.properties.imageFile);
        if (bitmapProxy && bitmapProxy.shouldDraw) {
            var thumbnailDiv = document.createElement("div");
            thumbnailDiv.id = "thumbnail_container";
            var thumbnail = document.createElement("img");
            thumbnail.id = 'thumbnail';
            thumbnail.src = bitmapProxy.image.src;
            thumbnailDiv.appendChild(thumbnail);
            details.appendChild(thumbnailDiv);
        }
    }

    updateCanvas();
  }


  var DrawTreeVisitor = function(canvas, drawFunc) {
    this._excludeSubtree = null;
    this._restrictToResourceSubtree = null;
    this._canvas = canvas;
    this._drawFunc = drawFunc;
  }

  DrawTreeVisitor.prototype = {
    Clone : function() {
      var clone = new DrawTreeVisitor(this._canvas, this._drawFunc);
      clone._excludeSubtree = this._excludeSubtree;
      clone._restrictToResourceSubtree = this._restrictToResourceSubtree;
      clone._suppressBitmaps = this._suppressBitmaps;
      clone._excludeChildren = this._excludeChildren;
      clone._excludeLinks = this._excludeLinks;
      clone._inIncludedSubtree = this._inIncludedSubtree;
      clone._currentVisual = this._currentVisual;
      clone._dstRect = this._dstRect;
      clone._bitmapDstRect = this._bitmapDstRect;
      clone._bitmapSourceRect = this._bitmapSourceRect;
      clone._clipPath = this._clipPath;
      clone._parentContext = this;
      return clone;
    },

    ApplyClip : function(drawFunc) {
      if (this._clipPath) {
        this._canvas.save();
        this._canvas.clip(this._clipPath, 'evenodd');
      }
      drawFunc(this);
      if (this._clipPath) {
        this._canvas.restore();
      }
    },

    PreSubgraph : function() {
      if ((this._node === this._excludeSubtree) ||
          (this._node._isVisual && !this._node._hasNonEmptyBounds) ||
          this._node._viewCollapsedOnAncestor) {
        // No reason to look at children or draw
        this._node = null;
        return;
      }

      if (!this._inIncludedSubtree) {
        this._inIncludedSubtree =
            (this._restrictToResourceSubtree === null) ||
            (this._restrictToResourceSubtree === this._node);
      }

      if (this._node._isVisual) {
        this._currentVisual = this._node;
        this._dstRect = undefined;
        this._bitmapDstRect = undefined;
        this._clipPath = undefined;
      }

      if (this._node._hasNonEmptyBounds) {
        if (isDefined(this._node._bounds)) {
          this._dstRect = this._node._bounds;

          // Window node's bitmaps aren't drawn to its bounds.
          if (isDefined(this._node._bitmapDstRect)) {
              this._bitmapDstRect = this._node._bitmapDstRect;
          }
        }
        else {
          this._dstRect = mapLocalBoundsToWorld(this._node._boundsInLocalSpace, this._currentVisual);
        }

        // We'll draw node bounds after all links/resources have had a chance to draw.
        // They may suppress the draw if they have more representative bounds.
        this._drawBounds = true;

        if (this._node._clipPath) {
          this._clipPath = this._node._clipPath;
        }
      }

      if (this._suppressBitmaps) {
        return;
      }

      // Source rect applies to the node it's defined on and CCompositionSurfaceBitmap
      // children.  it doesn't propagate to grandchildren.
      if (isDefined(this._node._sourceRect)) {
        this._bitmapSourceRect = this._node._sourceRect;
      }
      else if (this._node.name !== 'CCompositionSurfaceBitmap') {
        this._bitmapSourceRect = undefined;
      }

      if (!this._inIncludedSubtree) {
        // Don't draw bitmaps but keep going
        return;
      }

      if (isDefined(this._node._bitmapProxy) &&
          this._node._bitmapProxy.shouldDraw &&
          isDefined(this._dstRect)) {
        if (!isDefined(this._bitmapSourceRect)) {
            if (this._parentContext && this._parentContext._node._isVisual) {
            // Handle bitmaps as direct content of visuals: This is the classic DComp swapchain scenario
            // and the bitmap should be drawn in full.
            this._bitmapSourceRect = Rect.fromLTRB(0, 0, 
              this._node._bitmapProxy.image.naturalWidth, this._node._bitmapProxy.image.naturalHeight);
          }
          else if (this._currentVisual && (this._currentVisual.name === 'CWindowNode')) {
            // Handle window node sprite images, these should also be drawn in full.
            this._bitmapSourceRect = Rect.fromLTRB(0, 0, 
              this._node._bitmapProxy.image.naturalWidth, this._node._bitmapProxy.image.naturalHeight);
          }
        }

        if (isDefined(this._bitmapSourceRect)) {
          var srcRect = this._bitmapSourceRect;
          var dstRect = this._dstRect;
          
          if (isDefined(this._bitmapDstRect)) {
            dstRect = this._bitmapDstRect;
          }

          var drawImage = function(drawingContext) {
            drawingContext._canvas.drawImage(drawingContext._node._bitmapProxy.image,
                srcRect.left, srcRect.top, srcRect.width(), srcRect.height(),
                dstRect.left, dstRect.top, dstRect.width(), dstRect.height());
          }

          this.ApplyClip(drawImage);
        }
      }

      // Do not go into resources if this visual is collapsed. We handle this for visuals
      // with via _viewCollapsedOnAncestor, but resources are multi-instance.
      if (this._node._viewCollapsed) {
        // No reason to look at children - this node has been/will be drawn.
        this._node = null;
        return;
      }
    },

    PostSubgraph : function() {
      if (this._node._isVisual && this._drawBounds) {
        if (isDefined(this._parentContext) &&
            isDefined(this._parentContext._dstRect) &&
            isCloseReal(this._parentContext._dstRect.left, this._dstRect.left) &&
            isCloseReal(this._parentContext._dstRect.top, this._dstRect.top) &&
            isCloseReal(this._parentContext._dstRect.right, this._dstRect.right) &&
            isCloseReal(this._parentContext._dstRect.bottom, this._dstRect.bottom)) {
          // Parent should not draw bounds since it is identical to ours.
          this._parentContext._descendantDrewBounds = true;
        }
      }

      if (this._descendantDrewBounds) {
        this._drawBounds = false;
        if (!this._node._isVisual) {
          this._parentContext._descendantDrewBounds = true;
        }
      }

      if (this._restrictToResourceSubtree !== null) {
        if (this._inIncludedSubtree || this._ancestorOfIncludedSubtree) {
          if (!this._node._isVisual) {
            this._parentContext._ancestorOfIncludedSubtree = true;
          }
        }
        else {
          this._drawBounds = false;
        }
      }

      if (this._drawBounds) {
        // Invoke the render function if our children didn't say otherwise.
        this._drawFunc(this);

        if (!this._node._isVisual) {
          this._parentContext._descendantDrewBounds = true;
        }
      }
    }
  };


  var drawVisual = function(visitorFactory, visual, resource) {
    var drawVisual = visitorFactory();
    drawVisual._node = visual;
    drawVisual._restrictToResourceSubtree = resource;

    if (resource !== null) {
      drawVisual._excludeChildren = true;
    }
    else if (drawVisual._excludeChildren) {
      drawVisual._excludeLinks = true;
    }

    walkTree(drawVisual);
  }

  var drawResource = function(node, visitorFactory) {
    if (!node._isVisual) {
      for (visual of node._referencingVisuals) {
        drawVisual(visitorFactory, visual, node);
      }

      return;
    }

    drawVisual(visitorFactory, node, null);
  }


  var drawIsOutOfBounds = function(canvas, bounds) {
    var isAbove = bounds.bottom <= canvasBounds.top;
    var isBelow = bounds.top >= canvasBounds.bottom;
    var isToLeft = bounds.right <= canvasBounds.left;
    var isToRight = bounds.left >= canvasBounds.right;

    if (isAbove || isBelow || isToLeft || isToRight) {
      var thickness = 20 / canvasScale;
      var innerRect = Rect.fromLTRB(
          canvasBounds.left + thickness,
          canvasBounds.top + thickness,
          canvasBounds.right - thickness,
          canvasBounds.bottom - thickness);
      var path = new Path2D;

      if (isAbove) {
        path.moveTo(innerRect.left, innerRect.top);
        path.lineTo(innerRect.right, innerRect.top);
        path.lineTo((innerRect.left + innerRect.right) / 2, canvasBounds.top);
        path.closePath();
      }
      if (isBelow) {
        path.moveTo(innerRect.left, innerRect.bottom);
        path.lineTo(innerRect.right, innerRect.bottom);
        path.lineTo((innerRect.left + innerRect.right) / 2, canvasBounds.bottom);
        path.closePath();
      }
      if (isToLeft) {
        path.moveTo(innerRect.left, innerRect.top);
        path.lineTo(innerRect.left, innerRect.bottom);
        path.lineTo(canvasBounds.left, (innerRect.top + innerRect.bottom) / 2);
        path.closePath();
      }
      if (isToRight) {
        path.moveTo(innerRect.right, innerRect.top);
        path.lineTo(innerRect.right, innerRect.bottom);
        path.lineTo(canvasBounds.right, (innerRect.top + innerRect.bottom) / 2);
        path.closePath();
      }

      canvas.fill(path);
      return true;
    }

    return false;
  }


  var drawNodeTree = function(root, canvas) {
    canvas.strokeStyle = "rgba(200, 200, 200, 0.2)"; // gray
    canvas.lineWidth = 1 / canvasScale;

    var drawOutline = function(drawingContext) {
      const bounds = drawingContext._dstRect;
      drawingContext._canvas.strokeRect(bounds.left, bounds.top, bounds.width(), bounds.height());
    }

    // Draw the tree except the subtree under the hovered node if we have bitmaps
    var drawTree = new DrawTreeVisitor(canvas, drawOutline);
    drawTree._node = root;
    drawTree._excludeSubtree = hasValidBitmapsToDraw() ? hoveredNode : null;
    walkTree(drawTree);

    // Now draw all the hovered / selected tint effects (so they show on top of the bitmaps)
    if (hoveredNode != null) {
      if (hasValidBitmapsToDraw()) {
        var topmostBitmapsVisitor = function() {
          var draw = new DrawTreeVisitor(canvas, drawOutline);
          return draw;
        }

        drawResource(hoveredNode, topmostBitmapsVisitor);
      }

      canvas.fillStyle = "rgba(200, 200, 0, 0.2)"; // yellow-ish

      var highlightsVisitor = function() {
        var drawHoverHighlight = function(drawingContext) {
          const bounds = drawingContext._dstRect;
          if (!drawIsOutOfBounds(drawingContext._canvas, bounds)) {
            var drawWithClip = function(drawingContext) {
              drawingContext._canvas.fillRect(bounds.left, bounds.top, bounds.width(), bounds.height());
            }
            drawingContext.ApplyClip(drawWithClip);
          }
        }
  
        var draw = new DrawTreeVisitor(canvas, drawHoverHighlight);
        draw._suppressBitmaps = true;
        return draw;
      }

      // Show the hover highlight on all descendants of the node.  If the node is a visual,
      // this means all other visuals in its subtree.  If it is a link/resource, this means
      // all the visuals that reference the resource, either directly or indirectly via another
      // resource.
      drawResource(hoveredNode, highlightsVisitor);
    }

    if (selectedNode != null) {
      canvas.strokeStyle = "rgba(200, 0, 0, 1)"; // red
      canvas.fillStyle = "rgba(200, 0, 0, 0.1)"; // red

      var selectionVisitor = function() {
        var drawSelection = function(drawingContext) {
          const bounds = drawingContext._dstRect;
          if (!drawIsOutOfBounds(drawingContext._canvas, bounds)) {
            var drawWithClip = function(drawingContext) {
              drawingContext._canvas.fillRect(bounds.left, bounds.top, bounds.width(), bounds.height());
            }
            drawingContext.ApplyClip(drawWithClip);
            drawingContext._canvas.strokeRect(bounds.left, bounds.top, bounds.width(), bounds.height());
          }
        }
  
        var draw = new DrawTreeVisitor(canvas, drawSelection);
        draw._suppressBitmaps = true;
        draw._excludeChildren = true;
        return draw;
      }

      // Show the selection for this node.  If the node is a visual, this means just a single
      // rect for the bounds of the visual.  If it is a link/resource, this means a rect for
      // all visuals that reference it.
      drawResource(selectedNode, selectionVisitor);
    }
  }

  ///////////////////////////////////////////////////////////
  //
  // updateCanvas
  //
  ///////////////////////////////////////////////////////////

  var startX, startY;
  var canvasOffsetX = 0, canvasOffsetY = 0, canvasScale = 0.5;
  var canvasBounds;
  var hookedCanvasEvents = false;
  var updateCanvas = function() {
    var canvas = document.getElementById("canvas");
    var canvasHolder = document.getElementById("canvasHolder");

    if (!hookedCanvasEvents) {
      hookedCanvasEvents = true;

      canvas.addEventListener("mousedown", function(e) {
        canvas.focus();

        if (e.button == 0) {
          startX = e.screenX;
          startY = e.screenY;

          document.addEventListener("mouseup", mouseup, true);
          document.addEventListener("mousemove", mousemove, true);
          return false;
        }
      }, false);

      var mouseup = function(e) {
        if (e.button == 0) {
          document.removeEventListener("mouseup", mouseup, true);
          document.removeEventListener("mousemove", mousemove, true);
          return false;
        }
      };

      var mousemove = function(e) {
        canvasOffsetX += (e.screenX - startX) / canvasScale;
        canvasOffsetY += (e.screenY - startY) / canvasScale;

        startX = e.screenX;
        startY = e.screenY;

        updateCanvas();
        return false;
      };

      canvas.addEventListener("mousewheel", function(e) {
        var wheelDelta = e.wheelDelta;

        while (wheelDelta > 0) {
          canvasScale *= 1.2;
          wheelDelta -= 120;
        }
        while (wheelDelta < 0) {
          canvasScale /= 1.2;
          wheelDelta += 120;
        }

        updateCanvas();
      }, false);

      canvas.onkeydown = function(e) {
        var canvas = document.getElementById("canvas");

        switch (e.keyCode) {
            case 187: // +
                canvasScale *= 1.2;
                updateCanvas();
                break;

            case 189: // -
                canvasScale /= 1.2;
                updateCanvas();
                break;

            case 37: // Left arrow
                canvasOffsetX += (canvas.offsetWidth / 5);
                updateCanvas();
                break;

            case 39: // Right arrow
                canvasOffsetX -= (canvas.offsetWidth / 5);
                updateCanvas();
                break;

            case 38: // Up arrow
                canvasOffsetY += (canvas.offsetHeight / 5);
                updateCanvas();
                break;

            case 40: // Up arrow
                canvasOffsetY -= (canvas.offsetHeight / 5);
                updateCanvas();
                break;
        }
      };
    }

    canvas.width = canvasHolder.clientWidth;
    canvas.height = canvasHolder.clientHeight;

    var ctx = canvas.getContext('2d');

    // Apply user's translation and scale
    ctx.scale(canvasScale, canvasScale);
    ctx.translate(canvasOffsetX, canvasOffsetY);

    ctx.clearRect(0, 0, canvas.width, canvas.height);

    canvasBounds = Rect.fromLTRB(
        -canvasOffsetX,
        -canvasOffsetY,
        -canvasOffsetX + canvas.width / canvasScale,
        -canvasOffsetY + canvas.height / canvasScale);

    drawNodeTree(nodeTree, ctx);
  };

  window.onresize = updateCanvas;
</script>

<style type="text/css">
body {
  overflow: hidden;
}

html {
    font-family: "Segoe UI";
}

#tree {
    overflow: auto;
}

#details {
    overflow: auto;
}

splitter {
  display: block;
}

splitter > .pane1, splitter > .pane2 {
  position: absolute;
  top: 0;
  right: 0;
  bottom: 0;
  left: 0;
}

splitter > gripper {
  background: Gray;
  display: block;
  position: absolute;
  opacity: 0.3;
  top: 0;
  right: 0;
  bottom: 0;
  left: 0;
}

splitter.vertical > gripper {
  width: 6px;
  cursor: ew-resize;
}

splitter.horizontal > gripper {
  top: auto;
  height: 6px;
  cursor: ns-resize;
}

splitter.horizontal > .pane2 {
  top: auto;
}


treeviewitem {
    display: block;
    white-space: nowrap;
}

/* TreeViewItem text */
treeviewitem > span {
    cursor: pointer;
    color: Black;
}
treeviewitem.isvisual > span {
    color: Blue;
}
treeviewitem.isvisual:not(.hasbounds) > span {
    color:lightskyblue;
}
treeviewitem.hascontent > span {
    font-weight:bold;
}

treeviewitem > span:hover {
    color: rgb(200,200,0);
}
treeviewitem.selected > span:hover {
    color: rgb(200,100,0) !important;
}
treeviewitem.selected > span {
    color: rgb(200,0,0) !important;
    font-weight: bold;
}

.processblock {
  border-left: 1px solid lightgray;
  border-bottom: 1px solid lightgray;
  margin-bottom: -1px;
}

.processdetail {
  color: black;
  font-size: smaller;
}

.warning {
  color: red;
  font-size: smaller;
}

.warning a {
  color: red;
}

/* expando style */
treeviewitem > expando {
    float: left;
    width: 15px;
    padding-top: 5px;
    text-align: center;
    color: Black;
    font-size: 70%;
    cursor: pointer;
}
treeviewitem.lookatme > expando {
    color: red;
}

treeviewitem.empty > expando::before {
    visibility: hidden;
    content: "BACKSLASHHERE25E2";
}
treeviewitem.expanded > expando::before {
    content: "BACKSLASHHERE25E2";
}
treeviewitem.collapsed > expando::before {
    content: "BACKSLASHHERE25B7";
}

treeviewitem.cyclic {
    background-color: orange;
}

/* hide nested items if we are collapsed */
treeviewitem.collapsed > treeviewitem {
    display: none;
}

/* move nested items over a bit */
treeviewitem treeviewitem {
    margin-left: 15px;
}

#details {
    box-sizing: border-box;
    padding: 10px;
}

div.pane2 table td {
  vertical-align : top;
}

table.properties td {
    padding-left : 0.5em;
    padding-right : 1em;
}

table.properties tr td:first-child {
    font-weight: bold;
}

table.properties tr:nth-child(odd) {
    background: #E8EEF7;
}

div.detailcontent {
  padding-bottom: 0.5em;
  border-bottom: 1px solid black;
  margin-bottom: 0.5em;
}

#image_container {
    display         : none;
}

#thumbnail_container {
    margin-top: 5px;
}

#thumbnail {
    max-width: 100%;
    border: 1px solid red;

    /* Checkered image to show transparency */
    background-image: url('data:image/png;base64,iVBORw0KGgoAAAANSUhEUgAAAEgAAABICAIAAADajyQQAAAAAXNSR0IArs4c6QAAAARnQU1BAACxjwv8YQUAAAAJcEhZcwAADsMAAA7DAcdvqGQAAAAZdEVYdFNvZnR3YXJlAHBhaW50Lm5ldCA0LjAuMTM0A1t6AAAGHUlEQVRoQ+3Z6ZLbNhCFUb3/U3riLSPvyzjxngNcGJZIkKGrkqpMBd8PFkQ0+/ZtgBI5c3rx4sWjR4+ePn365MmTx48f397eGgxJDMS8f//++/fv3759+/r1q8EWAr58+fL582dhjq9fv3b57yN68gXRkiepknMHQtH6Pxkz3uJSLBqyVAtjBBw3NmQaa1RfG8aa1IrE4N8wtsX5fJ7GCtXXhrEhNP5tYxFa8OzZs2msUH1VYy9fvuRH3tRt6SOzILOUBBt8+PCha1QLmwhIQfTevn0rT8t4jK2tWHMPyKzIaWwaWxAZg2nsimmskdlp7J4a8wP94MGDm5sbRRPeQSQiptAkkmWLxCTs06dPr169aokOEy3X9jzJ6dg0rnFezMePH09v3rzx8BEI7yPm+fPnjq66u7v7syLLGucFwNomzMCKtUS/gqt4+6MiCQwkbErXmCL67t27k2sEeQbRFRO1hjEJcCUl3jzsMAlNHZLZjhKj9UuwkdaTg0EwbjIjzBZjVjALjaz1kB5AzJXZlnaLG2+N2yP3JMQYu0QvW67D6LgKJXn48OHld8G+ruNJ+62DclP0Dt2YeFX+VunVL4iAMGvlo7FGMpZUx+HNHpGBVr4CpMKWsU4xZsUtWrLs043lu9SKRWYIbcaybfh0zO5ouQ6jPIXSkkR3pEUatyYxKFvRcvfVkKg0akU0EuNm8wLSU6t4TSpQUMJ0wUkrlg4eR7wKJWFMNmllkzkqa8SYLcbcMHGF5GofrnH+0ph1lkK5qO6WZEo1tpAfEncIVVpb+bdgzJdHEpaFOJ8N0rIILaBISyNOdjCxNLIl20BA1pbDbEWJsiuGmEoFSJhv4SMqqPujoImM5XKuasoyznFNAkROY9NYJxrTWEMA4grTWCMB01hFwH0y5keTq5Zm47EDmY22ATG/g358e64FPPuthAEZ3hwZSwePw5gmSkIufUxag7RsgXqEOZ5cmT4p14KgWhgTMcV5pLJiBJJrSJ5xPAQ7pg4PRxE6TlYsayWDQZIXEyNMiaR48oiZdUBU24cNxBDzcsWVFavtG6DB6XGUnHFkrGU5DC2P0ZIkVbLBR51dE3UO24um6ztZtyFmPdrbvVZMV/RvizTPsYepj5bLW65j+AqwFbPs5XX1Bz4OiZaA8tqSNwvkgswNSUzehfdbAIvvWHtVUKJNtZ9/jdpcqB0dqTKIyhBL1f5KZds4ZpWzmlsIU9zlO8EO2duOxrkzW5bDWPPF66JBxpEYwtvJCpS9Wd+606S6k8ewLZJY7hYCl9+olzgve+9uIq1Yv1UOoh5aUsmQhD254xABFE95ZQxZsR2UpYsGl8Ych9TelZgU4WjbH1G5hDG7Q61JdZm29G+EWXKDP5hakDUChJkVZsyYFF1mBzFkDMQzlsuPkxUrhn7kybjmHmPWwk5j09gCMdPYgGnsJ2ansftozIOfij2ng7GWb0VsN5e3t0qMMceW75pk72Moi1ZU1qhhCGnGkoeWX/meMCeH0Cr/H1NosstiXI2MYS8tz7swtow57xkvpXiKzQuE3eGh1KPmGiuzhQqj4tgXbUeXIk5K9LiYukOqH6KFeTC9u7uTZUcgJIClDDwc9SYuyPqsMWWdFdqzXQ7WiEwTy2uLriS1o+pbD68RY0q/7SXF9ad7Gro4xBQodWOaKInL17SmrjDlaTbGOsnWZK6hhbJiLkvF4M14CzIJ4zP/6Wo62xRnFWOqNlVqXVNdDNBQ7Rgaax+uyVQxZqFdnOwGOyQSbhVivFl0KdKqNQQ6PmqkrwHXJsmCFLDGbeIqd2mWIuRVKCoL1CNAfNmKSjSyuwxkMTfEBQJYcknqs3QWPJWt8fKbV/KUaKA18qesBXqkhjUUyVlq1xINxpI3mWsoKgnlbx7ZKlnHZmJEAmJPipubm9xvKl4jwO510/qyEQn7ilZr7IqmsYI3NiSUrX6bFKSVLUIL1GOW+n/lD6bOD6FouXJ5mpVxjkMyNY1NY51SwjTm/JBp7CeZmsburzHlJtffIpgYY358k2tIreEfMObJIwnl0cp00zEnF1TN6/9oTmPTWGUaGzONNarmNLbgHhg7n/8CoYKbaGMlX9kAAAAASUVORK5CYII=')
}

</style>

<div style="display:none"><object id="json_data2" data="xamlsplore.json"/></div>
<script id="json_data" type="text/json">`;

    // Output the string.
    s = s.replace(/BACKSLASHHERE/g, "\\"); // put back the backslashes that the debugger js doesn't like
    var lines = s.split("\n");
    for (var line in lines)
    {
        if (outputLines)
        {
            outputLines.push(lines[line]);
        }
        else
        {
            host.diagnostics.debugLog(lines[line] + "\n");
        }
    }
}

function dumpEndHTML(outputLines)
{
    var sEnd = String.raw`
</script>
  
</html>
<!--`;

    // output the string
    var lines = sEnd.split("\n");
    for (var line in lines)
    {
        if (outputLines)
        {
            outputLines.push(lines[line]);
        }
        else
        {
            host.diagnostics.debugLog(lines[line] + "\n");
        }
    }
}

function __GetXamlThreadScript()
{
    var xamlthreadScript;
    try
    {
        // Look for the public winui-dbgext script first
        for (var script in host.namespace.Debugger.State.Scripts)
        {
            if (script.startsWith("winui-dbgext") || script.startsWith("winui_dbgext"))
            {
                xamlthreadScript = host.namespace.Debugger.State.Scripts[script];
                break;
            }
        }
        if (xamlthreadScript == undefined)
        {
            host.diagnostics.debugLog("Warning: Unable to find winui-dbgext script. Functionality reduced.\n");
            host.diagnostics.debugLog("         Load it with: .scriptrun <path>\\winui-dbgext.js\n");
        }
    }
    catch (ignored)
    {
        host.diagnostics.debugLog("Unable to find winui-dbgext script: " + ignored + "\n");
    }
    return xamlthreadScript;
}

class Rect
{
    toString()
    {
        return "(" + this.left + "," + this.top + " " + this.width + "x" + this.height + ")";
    }

    toJsonWH()
    {
        return '{"left":' + this.left + ',"top":' + this.top + ',"width":' + this.width + ',"height":' + this.height + '}';
    }
}

class SploreProperty
{
    constructor(name, value)
    {
        this.Name = name;
        this.Value = value;
    }

    toString() { return "SploreProperty: " + this.toJson(); }

    toJson()
    {
        return '"' + this.Name + '":"' + this.EscapeString(this.Value) + '"';
    }

    EscapeString(textToEscape)
    {
        // Encode unicode characters and escape problem characters in the text
        var text = "";
        for (var idx in textToEscape)
        {
            var c = textToEscape[idx];
            var code = textToEscape.charCodeAt(idx);
            if (c == '<')
            {
                text = text + "&lt;";
            }
            else if (c == '>')
            {
                text = text + "&gt;";
            }
            else if (c == '&')
            {
                text = text + "&amp;";
            }
            else if (c == '"')
            {
                text = text + "&quot;";
            }
            else if (c == '\\')
            {
                text = text + "\\\\";
            }
            else if (c == '\t')
            {
                text = text + "\\t";
            }
            else if (c == '\n' || c == '\r')
            {
                text = text + "\\n";
            }
            else if (code < 256)
            {
                text = text + c;
            }
            else
            {
                text = text + "&#" + code + ";";
            }
        }
        return text;
    }
}

class SploreNode
{
    constructor()
    {
        this.children = [];
        this.props = [];
    }

    toJson()
    {
        var json = "";
        json += '{"name":"' + this.Name + '"';
        json += ',"address":"' + this.Address + '"';

        // Properties
        json += ',"properties":{';
        if (this.Bounds != undefined)
        {
            json += '"ActualRect":' + this.Bounds.toJsonWH();
        }
        if (this.Id != undefined)
        {
            json += ',"ID":"' + this.Id + '"';
        }
        if (this.uielement != undefined && !this.uielement.isNull)
        {
            json += ',"m_fVisibility":"' + this.uielement.m_fVisibility + '"';
        }
        if (this.Text != undefined)
        {
            var prop = new SploreProperty("Text", this.Text);
            json += ',' + prop.toJson();
            json += ',"FontFamily":"' + this.FontFamily + '"';
        }
        for (var prop of this.props)
        {
            json += ',' + prop.toJson();
        }
        json += '}';

        // Children
        json += ',"children":[';
        for (var i = 0; i < this.children.length; i++)
        {
            var child = this.children[i];

            if (i > 0)
            {
                json += ",";
            }

            json += '"' + child.Address + '"';
        }
        json += ']';

        json += '}';
        return json;
    }
}

// SploreData, used by !xamlsplore
class SploreData
{
    constructor(optionElementAddress)
    {
        this.__Initialize(optionElementAddress);
    }

    /*
    toString()
    {
        return this.__errorCodeString;
    }

    get ErrorCode()
    {
        return this.__errorCode;
    }

    get ErrorCodeString()
    {
        return this.__errorCodeString;
    }
    */

    __Initialize(optionElementAddress)
    {
        var xamlthreadScript = __GetXamlThreadScript();

        // Walk the XAML tree using public symbols (TLS → DXamlCore → VisualTree → m_pChildren)
        var rootElement = this.__FindRootElement(optionElementAddress);
        if (rootElement == null)
        {
            throw new Error("Can't find root element. Ensure XAML symbols are loaded and a XAML thread exists.");
        }
        host.diagnostics.debugLog("Found root element at 0x" + rootElement.address.toString(16) + "\n");
        host.diagnostics.debugLog("Walking tree...\n");
        this.rootNode = this.__WalkElementTree(rootElement, xamlthreadScript);
        if (this.rootNode == null)
        {
            throw new Error("Can't find root node. The root element at 0x" + rootElement.address.toString(16) + " could not be walked. Consider passing in a pointer.");
        }
        host.diagnostics.debugLog("Tree walk complete.\n");
    }

    // Find the root XAML element using public symbols
    __FindRootElement(optionElementAddress)
    {
        if (optionElementAddress != undefined)
        {
            var addr = host.evaluateExpression("0x" + optionElementAddress.toString(16));
            return host.createPointerObject(addr, XAML_DLL_Name(), "CUIElement*");
        }

        // Walk threads to find the DXamlCore via TLS, then get the root visual
        for (var thread of host.currentProcess.Threads)
        {
            try
            {
                var ptr = GetTLSPointer(thread, XAML_DLL_Name(), "DXamlInstanceStorage::g_dwTlsIndex");
                if (ptr == null || ptr.isNull)
                {
                    continue;
                }

                // The TLS slot contains a pointer to DXamlCore. Use ptr.address to get the address value.
                var dxamlCore = host.createPointerObject(ptr.address, XAML_DLL_Name(), "DirectUI::DXamlCore*");
                if (dxamlCore.isNull)
                {
                    continue;
                }

                var visualTree = dxamlCore.m_hCore.m_pMainVisualTree;

                // Try m_rootVisual.m_ptr (xref_ptr<CRootVisual>) — the full visual tree root
                var rootVisual = null;
                try
                {
                    rootVisual = visualTree.m_rootVisual.m_ptr;
                }
                catch (e)
                {
                    // Fallback: some builds use m_pRootVisual directly
                    try
                    {
                        rootVisual = visualTree.m_pRootVisual;
                    }
                    catch (e2)
                    {
                    }
                }

                if (rootVisual != null && !rootVisual.isNull)
                {
                    // Cast to CUIElement* for consistent access
                    var rootElement = host.createPointerObject(rootVisual.address, XAML_DLL_Name(), "CUIElement*");
                    try
                    {
                        var typedObj = rootElement.dereference().runtimeTypedObject;
                    }
                    catch (e) {}

                    // Check if root has children
                    try
                    {
                        var pChildren = rootElement.m_pChildren;
                    }
                    catch (e)
                    {
                    }

                    return rootElement;
                }

                // If no root visual, try public root visual
                try
                {
                    rootVisual = visualTree.m_publicRootVisual.m_ptr;
                    if (rootVisual != null && !rootVisual.isNull)
                    {
                        return host.createPointerObject(rootVisual.address, XAML_DLL_Name(), "CUIElement*");
                    }
                }
                catch (e3)
                {
                }

                try
                {
                    rootVisual = visualTree.m_pPublicRootVisual;
                    if (rootVisual != null && !rootVisual.isNull)
                    {
                        return host.createPointerObject(rootVisual.address, XAML_DLL_Name(), "CUIElement*");
                    }
                }
                catch (e4)
                {
                }
            }
            catch (ignored)
            {
            }
        }

        return null;
    }

    // Recursively walk the XAML element tree using m_pChildren
    __WalkElementTree(element, xamlthreadScript)
    {
        if (element == null || element.isNull)
        {
            return null;
        }

        var newNode = new SploreNode();
        newNode.Address = "0x" + element.address.toString(16);

        // Get the type name
        try
        {
            var typedObj = element.dereference().runtimeTypedObject;
            var typeName = typedObj.targetType;
            // Extract just the class name (e.g. "CButton" from "Microsoft_UI_Xaml!CButton")
            var bangIdx = typeName.indexOf('!');
            if (bangIdx >= 0)
            {
                typeName = typeName.substring(bangIdx + 1);
            }
            // Remove pointer suffix
            if (typeName.endsWith(" *"))
            {
                typeName = typeName.substring(0, typeName.length - 2);
            }
            newNode.RawName = typeName;
            newNode.Name = typeName;
        }
        catch (e)
        {
            // runtimeTypedObject failed (e.g. app-defined types like MainWindow).
            // Fall back to vtable lookup via dps/printf to get the type name.
            var inferredName = null;
            try
            {
                var potentialObject = host.createPointerObject(element.address, "combase.dll", "void**");
                var potentialVtable = potentialObject.dereference();
                var output = host.namespace.Debugger.Utility.Control.ExecuteCommand(
                    ".printf \"%ly\", 0x" + potentialVtable.address.toString(16));
                for (var line of output)
                {
                    var vftIdx = line.indexOf("::`vftable'");
                    if (vftIdx > 0)
                    {
                        inferredName = line.substring(0, vftIdx);
                        // Extract class name after "!" if present
                        var bangIdx = inferredName.indexOf('!');
                        if (bangIdx >= 0)
                        {
                            inferredName = inferredName.substring(bangIdx + 1);
                        }
                        break;
                    }
                }
            }
            catch (vtableErr) {}

            newNode.RawName = inferredName || "Unknown";
            newNode.Name = inferredName || "Unknown";
        }

        // Get bounds from layout storage
        newNode.uielement = element;
        try
        {
            if (!element.isNull)
            {
                newNode.Bounds = new Rect();
                newNode.Bounds.left = element.m_layoutStorage.m_offset.x;
                newNode.Bounds.top  = element.m_layoutStorage.m_offset.y;
                newNode.Bounds.width  = element.m_layoutStorage.m_size.width;
                newNode.Bounds.height = element.m_layoutStorage.m_size.height;
            }
        }
        catch (e)
        {
            newNode.Bounds = new Rect();
            newNode.Bounds.left = 0;
            newNode.Bounds.top  = 0;
            newNode.Bounds.width  = 0;
            newNode.Bounds.height = 0;
        }

        // Try to get enriched element info from winui-dbgext script
        try
        {
            if (xamlthreadScript != undefined)
            {
                var enrichedElement = xamlthreadScript.Contents.dumpXamlElement(element.address);
                if (enrichedElement != null)
                {
                    var className = enrichedElement.GetShortDerivedName();
                    if (className != undefined)
                    {
                        newNode.Name = className;
                    }
                    for (var prop of enrichedElement.SparseProperties)
                    {
                        var sploreProp = new SploreProperty(prop.PropertyName, prop.ValueToString());
                        newNode.props.push(sploreProp);
                    }
                }
            }
        }
        catch (ignored) {}

        // Get text content for TextBlock
        try
        {
            // TODO: Include the Text Property as well
            if (newNode.RawName == "TextBlock" && !element.isNull)
            {
                if (!element.m_pTextFormatting.isNull)
                {
                    var fontFamily = element.m_pTextFormatting.m_pFontFamily.m_pFontFamilyName.m_strName.toString();
                    if (fontFamily.startsWith('"'))
                    {
                        fontFamily = fontFamily.split('"')[1];
                    }
                    newNode.FontFamily = fontFamily;
                }
            }
        }
        catch (e) {}

        // Get the element name (x:Name)
        try
        {
            var name = element.m_strName.toString();
            if (name != "" && name != '""')
            {
                newNode.Id = name.replace(/"/g, '');
            }
        }
        catch (e) {}

        // Walk children
        try
        {
            var pChildren = element.m_pChildren;
            if (!pChildren.isNull)
            {
                var children = pChildren;

                // CDOCollection.m_items is a CompactInlineVector<CDependencyObject*, 2>.
                // WinDbg JS can't call C++ operator[], so we access the raw m_data pointer
                // and index into it directly. m_data is CDependencyObject** (a raw pointer array).
                var walked = false;

                // Approach 1: CompactInlineVector — m_items.m_data[i] with m_items.m_size
                if (!walked)
                {
                    try
                    {
                        var compactVec = children.m_items;
                        var count = compactVec.m_size;
                        if (count > 0 && count < 10000)
                        {
                            var dataPtr = compactVec.m_data;
                            for (var i = 0; i < count; i++)
                            {
                                try
                                {
                                    var childPtr = dataPtr[i];
                                    if (childPtr != null && !childPtr.isNull)
                                    {
                                        var childElement = host.createPointerObject(childPtr.address, XAML_DLL_Name(), "CUIElement*");
                                        var childNode = this.__WalkElementTree(childElement, xamlthreadScript);
                                        if (childNode != null)
                                        {
                                            newNode.children.push(childNode);
                                        }
                                    }
                                }
                                catch (childErr)
                                {
                                }
                            }
                            walked = true;
                        }
                    }
                    catch (e1)
                    {
                    }
                }

                // Approach 2: Try direct indexing on m_items (works if WinDbg has NatVis for CompactInlineVector)
                if (!walked)
                {
                    try
                    {
                        var firstChild = children.m_items[0];
                        if (firstChild != null)
                        {
                            var idx = 0;
                            while (true)
                            {
                                try
                                {
                                    var childPtr = children.m_items[idx];
                                    if (childPtr == null) break;
                                    if (!childPtr.isNull)
                                    {
                                        var childElement = host.createPointerObject(childPtr.address, XAML_DLL_Name(), "CUIElement*");
                                        var childNode = this.__WalkElementTree(childElement, xamlthreadScript);
                                        if (childNode != null)
                                        {
                                            newNode.children.push(childNode);
                                        }
                                    }
                                    idx++;
                                    if (idx > 10000) break;
                                }
                                catch (idxErr) { break; }
                            }
                            walked = true;
                        }
                    }
                    catch (e2)
                    {
                    }
                }

                // Approach 3: Legacy m_nItemCount + m_items as raw array
                if (!walked)
                {
                    try
                    {
                        var count = children.m_nItemCount;
                        if (count > 0 && count < 10000)
                        {
                            for (var i = 0; i < count; i++)
                            {
                                try
                                {
                                    var childPtr = children.m_items[i];
                                    if (childPtr != null && !childPtr.isNull)
                                    {
                                        var childElement = host.createPointerObject(childPtr.address, XAML_DLL_Name(), "CUIElement*");
                                        var childNode = this.__WalkElementTree(childElement, xamlthreadScript);
                                        if (childNode != null)
                                        {
                                            newNode.children.push(childNode);
                                        }
                                    }
                                }
                                catch (childErr) {}
                            }
                            walked = true;
                        }
                    }
                    catch (e3)
                    {
                    }
                }

                // Approach 4: WinDbg iterable interface
                if (!walked)
                {
                    try
                    {
                        for (var child of children)
                        {
                            try
                            {
                                if (child != null && !child.isNull)
                                {
                                    var childElement = host.createPointerObject(child.address, XAML_DLL_Name(), "CUIElement*");
                                    var childNode = this.__WalkElementTree(childElement, xamlthreadScript);
                                    if (childNode != null)
                                    {
                                        newNode.children.push(childNode);
                                    }
                                }
                            }
                            catch (childErr) {}
                        }
                    }
                    catch (iterErr)
                    {
                    }
                }

                if (!walked && newNode.children.length == 0)
                {
                }
            }
        }
        catch (e)
        {
        }

        return newNode;
    }

    PrintInternal(node, level)
    {
        for (var i = level; i > 0; i--)
        {
            host.diagnostics.debugLog("  ");
        }

        host.diagnostics.debugLog(node.Name + " " + node.Address + " Bounds: " + node.Bounds + "\n");

        //for (var child in node.children)
        for (var i = 0; i < node.children.length; i++)
        {
            var child = node.children[i];
            this.PrintInternal(child, level+1);
        }
    }

    __PutTreeNodesInList(node, list)
    {
      if (node == undefined)
      {
        __debugDump(node);
        return;
      }

        list.push(node);

        //for (var child in node.children)
      if (node.children == undefined)
        {
            __debugDump(node);
        return;
        }
        for (var i = 0; i < node.children.length; i++)
        {
            var child = node.children[i];
            this.__PutTreeNodesInList(child, list);
        }
    }

    PrintJson(printMultiLine, outputLines)
    {
        var list = [];
        this.__PutTreeNodesInList(this.rootNode, list);

        var json = '{"nodes":[';
        //for (var child in node.children)
        for (var i = 0; i < list.length; i++)
        {
            var node = list[i];

            if (i > 0)
            {
                json += ",";
            }
            if (printMultiLine && i > 0)
            {
                if (outputLines)
                {
                    outputLines.push(json);
                }
                else
                {
                    host.diagnostics.debugLog(json + "\n");
                }
                json = "";
            }
            /*
            if (i == 0)
            {
                __debugDump(node.uielement);
                break;
            }
            */


            json += node.toJson();
            //host.diagnostics.debugLog(node.Name + " " + node.Address + " Bounds: " + node.Bounds + "\n");
        }
        json += ']}';
        if (outputLines)
        {
            outputLines.push(json);
        }
        else
        {
            host.diagnostics.debugLog(json + "\n");
        }

    }

    Print()
    {
        if (this.rootNode != null)
        {
            this.PrintInternal(this.rootNode, 0);
            host.diagnostics.debugLog("-------\n");
            this.PrintJson();
        }
    }
}

// !xamlsplore function
function xamlsplore()
{
    // !xamlsplore requires XAML symbols to be loaded.
    try
    {
        __CheckXAMLSymbolsLoaded();
    }
    catch(e)
    {
        host.diagnostics.debugLog("Error: XAML symbols not loaded.\n");
        return;
    }

    var outputPath = "c:\\dumps\\splore.html";
    var sploreData = new SploreData();
    var html = [];
    dumpStartHTML(html);
    sploreData.PrintJson(true, html);
    dumpEndHTML(html);

    var fs = host.namespace.Debugger.Utility.FileSystem;
    var file = fs.CreateFile(outputPath, "CreateAlways");
    var writer = fs.CreateTextWriter(file);
    for (var i = 0; i < html.length; i++)
    {
        if (html[i].length > 0)
        {
            writer.WriteLine(html[i]);
        }
    }
    file.Close();
    host.diagnostics.debugLog("Output written to " + outputPath + "\n");
}

// !xamlsploredumper function
function xamlsploredumper(onlyPrintJson)
{
    // !xamlsploredumper requires XAML symbols to be loaded.
    try
    {
        __CheckXAMLSymbolsLoaded();
    }
    catch(e)
    {
        host.diagnostics.debugLog("Error: XAML symbols not loaded.\n");
        return;
    }

    var sploreData = new SploreData();
    if (onlyPrintJson)
    {
        sploreData.PrintJson(true);
        return;
    }
    else
    {
        sploreData.Print();
    }
}

function initializeScript()
{
    return [
            new host.functionAlias(xamlsplore, "xamlsplore"),
            new host.functionAlias(xamlsploredumper, "xamlsploredumper")];
}
