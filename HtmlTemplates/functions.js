
var svgns = "http://www.w3.org/2000/svg";
var mouseX = 0, mouseY = 0;
var floatingTooltip = null;

var filterSourceFieldPredicates = {
	'none': function(node) { return 0; },
	'inclusive': function(node) { return node.profTimeTotalInclusive; },
	'inclusive-pct': function(node) { return node.profTimeTotalInclusivePct; },
	'exclusive': function(node) { return node.profTimeTotalExclusive; },
	'exclusive-pct': function(node) { return node.profTimeTotalExclusivePct; },
	'callcount': function(node) { return node.profTotalCallCount; },
	'inclusive-exclusive-ratio': function(node) { return (1.0+node.profTimeTotalInclusivePct)/(1.0+node.profTimeTotalExclusivePct); },
	'exclusive-inclusive-ratio': function(node) { return -(1.0+node.profTimeTotalExclusivePct)/(1.0+node.profTimeTotalInclusivePct); },
};

var filterOperatorPredicates = {
	'greaterthan': function(a,b) { return a > b; },
	'greaterequal': function(a,b) { return a >= b; },
	'lessthan': function(a,b) { return a < b; },
	'lessequal': function(a,b) { return a <= b; },
	'equals': function(a,b) { return a == b; }
};

function pivo_selectTab(identifier)
{
	$('.report-tab-content').addClass('hidden');
	$('.report-tab').removeClass('selected');
	
	$('#report-tab--'+identifier).removeClass('hidden');
	$('#report-tab-menu--'+identifier).addClass('selected');
}

function pivo_initMouseTrack()
{
	function handleMouseMove(event) {
		var dot, eventDoc, doc, body, pageX, pageY;
		event = event || window.event;

		if (event.pageX == null && event.clientX != null)
		{
			eventDoc = (event.target && event.target.ownerDocument) || document;
			doc = eventDoc.documentElement;
			body = eventDoc.body;

			event.pageX = event.clientX +
				(doc && doc.scrollLeft || body && body.scrollLeft || 0) -
				(doc && doc.clientLeft || body && body.clientLeft || 0);
			event.pageY = event.clientY +
				(doc && doc.scrollTop  || body && body.scrollTop  || 0) -
				(doc && doc.clientTop  || body && body.clientTop  || 0 );
		}

		mouseX = event.pageX;
		mouseY = event.pageY;
		
		floatingTooltip.css({'position': 'absolute', 'top': mouseY, 'left': mouseX});
	}

	document.onmousemove = handleMouseMove;
	floatingTooltip = $('<div class="custom-tooltip"></div>');
	$('body').append(floatingTooltip);
	floatingTooltip.hide();
}

function pivo_removeFilter(element)
{
	$(element).parent().parent().remove();
}

function pivo_addFilter(identifier)
{
	var nfe = $('.'+identifier+'-filter-entry.original').clone();
	nfe.removeClass('original');
	nfe.insertAfter($('.'+identifier+'-filter-entry').last());
	var delbtn = $('<span onclick="pivo_removeFilter(this);">✕</span>');
	nfe.find('.'+identifier+'-filter-deletefield').append(delbtn);
}

function pivo_createFlatView()
{
	var filterConditions = [];
	$.each($('.flatview-filter-entry'), function(i,e) {
		var type = $(e).find('.flatview-filter-type').val();
		var op = $(e).find('.flatview-filter-operator').val();
		var value = $(e).find('.flatview-filter-value').val();
		
		if (type !== 'none')
			filterConditions.push({ 'type': type, 'op': op, 'value': value });
	});

	$.each($('.flatview tbody tr'), function(i,e) {
		var passed = true;

		for (var j in filterConditions)
		{
			if (!(filterOperatorPredicates[filterConditions[j].op](parseFloat($(e).data(filterConditions[j].type)), filterConditions[j].value)))
			{
				passed = false;
				break;
			}
		}
		
		if (passed)
			$(e).show();
		else
			$(e).hide();
	});

	$('.flatview th.sort-float').data('sortBy', function(th, td, tablesort) {
		return parseFloat(td.text());
	});
	$('.flatview th.sort-int').data('sortBy', function(th, td, tablesort) {
		return parseInt(td.text(), 10);
	});
	$('.flatview th.sort-string').data('sortBy', function(th, td, tablesort) {
		return td.text();
	});
	$('.flatview').tablesort();
					
	$('.flatview th').click(function() {
		$('.flatview th').removeClass('sorted-by');
		$(this).addClass('sorted-by');
	});
}

var nodeInfo = {
};

var callGraphData = [
];

// key = node, value = array of incident nodes (from node to each of these nodes)
var incidencyGraph = {
};
// key = node, value = array of source ("parent") nodes (from each of these nodes to key node)
var reverseIncidencyGraph = {
};

var callTree = {
};

var callTreeMaxTime = 0.0;
// maximum depth in call tree (used later for flame graph)
var callTreeMaxDepth = 0;

function escapeHtml(txt)
{
	return $('<div>').text(txt).html();
}

function pivo_addCallGraphNode(id, name, timeTotalInclusive, timeTotalInclusivePct, timeTotal, timeTotalPct, totalCallCount, fnctype)
{
	if (typeof nodeInfo[id] === 'undefined')
	{
		var title = '<span class="fname">'+escapeHtml(name)+'</span><br/>'+
			    'Inclusive time: '+timeTotalInclusivePct+'% ('+timeTotalInclusive+'s)<br/>'+
			    'Exclusive time: '+timeTotalPct+'% ('+timeTotal+'s)<br/>'+
			    'Call count: '+totalCallCount+'x';

		nodeInfo[id] = {
			'id': id,
			'name': name,
			'profTimeTotalInclusive': timeTotalInclusive,
			'profTimeTotalInclusivePct': timeTotalInclusivePct,
			'profTimeTotalExclusive': timeTotal,
			'profTimeTotalExclusivePct': timeTotalPct,
			'profTotalCallCount': totalCallCount,
			'title': title,
			'functionType': fnctype
		};

		incidencyGraph[id] = [];
		reverseIncidencyGraph[id] = [];
	}
}

function pivo_createCallTreeNode(id, time, timepct, samples)
{
	return {
		'id': id,
		'timeTotal': parseFloat(time),
		'timeTotalPct': parseFloat(timepct),
		'sampleCount': parseFloat(samples),
		'children': { },
		'maxdepth': 1
	};
}

function pivo_addCallTreeChain(idchain, timechain, timepctchain, samplecountchain)
{
	var ids = idchain.split(',');
	var times = timechain.split(',');
	var timepcts = timepctchain.split(',');
	var samples = samplecountchain.split(',');

	if (ids.length === 0)
		return;

	if (ids.length !== times.length || ids.length !== timepcts.length || ids.length !== samples.length)
		return;
		
	if (ids.length > callTreeMaxDepth)
		callTreeMaxDepth = ids.length;

	if (typeof callTree[ids[0]] === 'undefined')
		callTree[ids[0]] = pivo_createCallTreeNode(ids[0], times[0], timepcts[0], samples[0]);

	var curr = callTree[ids[0]];
	for (var i = 1; i < ids.length; i++)
	{
		if (typeof curr.children[ids[i]] === 'undefined')
			curr.children[ids[i]] = pivo_createCallTreeNode(ids[i], times[i], timepcts[i], samples[i]);
			
		if (curr.maxdepth < ids.length - i)
			curr.maxdepth = ids.length - i;

		curr = curr.children[ids[i]];
	}
}

function pivo_createCallTreeRow(record, path, isroot)
{
	var classes = "call-tree-row" + (isroot ? " root-node" : "");
	var name = escapeHtml(((typeof nodeInfo[record.id] === 'undefined') ? "??" : nodeInfo[record.id].name));

	var r, g, b;
	b = 0;
	var pctTime = record.timeTotalPct / callTreeMaxTime;
	if (pctTime > 0.5)
	{
		r = 255;
		g = rangeAlignColor(Math.floor(255 * (0.5 - (pctTime - 0.5)) * 2));
	}
	else
	{
		r = rangeAlignColor(Math.ceil(255 * pctTime * 2));
		g = 255;
	}
	var color = pivo_getHexColor(r, g, b);
	var pcttxt = (100.0*record.timeTotalPct).toFixed(2);
	var off = 6 - pcttxt.length;
	for (var i = 0; i < off; i++)
		pcttxt = '&nbsp;' + pcttxt;

	var chtime = 0.0;
	var hasChildren = false;
	if (Object.keys(record.children).length > 0)
	{
		hasChildren = true;
		for (var i in record.children)
			chtime += record.children[i].timeTotalPct;
	}
	else
		chtime = record.timeTotalPct;

	var exclTime = record.timeTotalPct - chtime;
	if (exclTime < 0.0)
		exclTime = 0.0;

	pctTime = exclTime / record.timeTotalPct;
	if (pctTime > 0.5)
	{
		r = 255;
		g = rangeAlignColor(Math.floor(255 * (0.5 - (pctTime - 0.5)) * 2));
	}
	else
	{
		r = rangeAlignColor(Math.ceil(255 * pctTime * 2));
		g = 255;
	}
	var excolor = pivo_getHexColor(r, g, b);
	var pctextxt = hasChildren ? (100.0*exclTime).toFixed(2) : (100.0*record.timeTotalPct).toFixed(2);
	off = 6 - pctextxt.length;
	for (var i = 0; i < off; i++)
		pctextxt = '&nbsp;' + pctextxt;

	var pmb = hasChildren ? "+" : "&nbsp;";
	
	return '<div class="'+classes+'" data-path="'+path+'" onclick="pivo_expandCallTree(this, event);">'
		+'<span class="call-tree-pmbox">'+pmb+'</span>'
		+'<span class="call-tree-pctbox" style="background-color: '+color+'">'+pcttxt+'%</span>'
		+'<span class="call-tree-pctbox-ex" style="border-color: '+excolor+'">'+pctextxt+'%</span>'
		+'<span class="call-tree-txtbox">'+name+'</span>'
		+'</div>';
}

function pivo_callTreeSortChildren(arr)
{
	var sorted = [];
	var present = [];

	for (var i in arr)
	{
		var max = 0.0;
		var maxId = -1;
		for (var j in arr)
		{
			if (present.indexOf(j) !== -1 || arr[j].timeTotal < max)
				continue;
			max = arr[j].timeTotal;
			maxId = j;
		}

		sorted.push(arr[maxId]);
		present.push(maxId);
	}

	return sorted;
}

function pivo_createCallTree()
{
	var sorted = pivo_callTreeSortChildren(callTree);
	if (sorted.length > 0)
		callTreeMaxTime = sorted[0].timeTotalPct;

	for (var i in sorted)
	{
		$('#call-tree-target').append(pivo_createCallTreeRow(sorted[i], sorted[i].id, true));
	}
}

function pivo_expandCallTree(elem, event)
{
	event.stopPropagation();
	event.preventDefault();

	if ($(elem).data('expanded') === '1')
	{
		$(elem).children('.call-tree-row').remove();
		$(elem).data('expanded', '0');
		$(elem).children('.call-tree-pmbox').html('+');
		return;
	}

	var path = ""+$(elem).data('path');
	var pathIds = path.split(',');

	var curr = callTree[pathIds[0]];
	for (var i = 1; i < pathIds.length; i++)
		curr = curr.children[pathIds[i]];

	if (Object.keys(curr.children).length == 0)
		return;

	var vingl = $('<div class="call-tree-vingl"></div>');

	var sorted = pivo_callTreeSortChildren(curr.children);
	var fpass = true;

	for (var i in sorted)
	{
		var ins = $(pivo_createCallTreeRow(sorted[i], path+','+sorted[i].id));
		if (fpass)
		{
			ins.append(vingl);
			fpass = false;
		}
		$(elem).append(ins);
	}

	$(elem).data('expanded', '1');
	$(elem).children('.call-tree-pmbox').html('-');
}

function pivo_addCallGraphEdge(source, dest, callCount)
{
	callGraphData.push({ 'from': source, 'to': dest, 'arrows': 'to' });

	incidencyGraph[source].push(dest);
	reverseIncidencyGraph[dest].push(source);
}

function pivo_createFlameGraph(basePath)
{
	var borderWidth = 1;
	var boxh = 20;
	var spaceh = 0;
	var fullw = $('#flame-graph-target').outerWidth();
	var fullh = $('#flame-graph-target').outerHeight();
	var baseleft = fullw * 0.1;

	// reset width and height, just to be sure
	$('#flame-graph-target').attr('width', fullw);

	// cut some margin
	fullw -= baseleft * 2;

	var c = document.getElementById("flame-graph-target");
	c.innerHTML = '';
	var svgDoc = c.ownerDocument;

	var iterStack = [];
	var bl = baseleft;
	var refDepth = callTreeMaxDepth;

	// if the path is not specified, generate whole view
	if (typeof basePath === 'undefined')
	{
		for (var i in callTree)
		{
			callTree[i].flameGraphPath = i;
			callTree[i].flameGraphLeft = bl;
			callTree[i].flameGraphWidth = Math.round(fullw * callTree[i].timeTotalPct);
			bl += Math.round(fullw * callTree[i].timeTotalPct);
			iterStack.push(callTree[i]);
		}
	}
	else // otherwise try to expand from clicked node
	{
		// expand path to desired start
		var nodepath = basePath.split(',');
		var curr = callTree[nodepath[0]];
		refDepth = curr.maxdepth;
		for (var i = 1; i < nodepath.length; i++)
			curr = curr.children[nodepath[i]];

		// make this node expanded
		curr.flameGraphPath = basePath;
		curr.flameGraphLeft = bl;
		curr.flameGraphWidth = fullw;

		iterStack.push(curr);
	}

	// first line to begin generating flame graph from (entry points)
	var baseline = (boxh + spaceh) * (refDepth + 1);
	
	fullh = baseline + boxh;
	$('#flame-graph-target').height(fullh);
	$('#flame-graph-target').css('height', fullh+"px");

	// iterative BFS with level preservation needs arrays swapping
	var tmpStack = [];

	while (iterStack.length != 0)
	{
		for (var i in iterStack)
		{
			// push our children to temporary stack
			var bl = iterStack[i].flameGraphLeft;
			for (var j in iterStack[i].children)
			{
				iterStack[i].children[j].flameGraphPath = iterStack[i].flameGraphPath+','+iterStack[i].children[j].id;
				iterStack[i].children[j].flameGraphLeft = bl;
				iterStack[i].children[j].flameGraphWidth = Math.round(iterStack[i].flameGraphWidth * iterStack[i].children[j].timeTotalPct / iterStack[i].timeTotalPct);
				bl += Math.round(iterStack[i].flameGraphWidth * iterStack[i].children[j].timeTotalPct / iterStack[i].timeTotalPct);
				tmpStack.push(iterStack[i].children[j]);
			}

			// create element group with all needed info
			var gr = svgDoc.createElementNS(svgns, 'g');
			gr.setAttributeNS(null, 'call-tree-path', iterStack[i].flameGraphPath);
			gr.setAttributeNS(null, 'nodeid', iterStack[i].id);
			gr.setAttributeNS(null, 'samples', iterStack[i].sampleCount);
			gr.setAttributeNS(null, 'time-total-inclusive', iterStack[i].timeTotal.toFixed(2));
			gr.setAttributeNS(null, 'time-total-inclusive-pct', (100.0*iterStack[i].timeTotalPct).toFixed(2));

			// calculate exclusive time in this subtree
			var chtime = 0.0, chtimepct = 0.0;
			if (Object.keys(iterStack[i].children).length > 0)
			{
				for (var j in iterStack[i].children)
				{
					chtime += iterStack[i].children[j].timeTotal;
					chtimepct += iterStack[i].children[j].timeTotalPct;
				}
			}
			else
			{
				chtime = iterStack[i].timeTotal;
				chtimepct = iterStack[i].timeTotalPct;
			}
			var exclTime = iterStack[i].timeTotal - chtime;
			var exclTimePct = iterStack[i].timeTotalPct - chtimepct;
			if (exclTime < 0.0) exclTime = 0.0;
			if (exclTimePct < 0.0) exclTimePct = 0.0;

			gr.setAttributeNS(null, 'time-total-exclusive', exclTime.toFixed(2));
			gr.setAttributeNS(null, 'time-total-exclusive-pct', (100.0*exclTimePct).toFixed(2));

			// draw rectangle itself
			var rect = svgDoc.createElementNS(svgns, 'rect');
			rect.setAttributeNS(null, 'x', iterStack[i].flameGraphLeft);
			rect.setAttributeNS(null, 'y', baseline);
			rect.setAttributeNS(null, 'width', iterStack[i].flameGraphWidth);
			rect.setAttributeNS(null, 'height', boxh);
			rect.setAttributeNS(null, 'style',  'fill:'+pivo_getHexColor(Math.ceil(255*(0.7+0.3*Math.random())), Math.ceil(255*(0.3+0.4*Math.random())), 0)+';'+
												'cursor:pointer;'+
												'stroke:#FFE4BC; stroke-width:'+borderWidth+';');
			gr.appendChild(rect);

			c.appendChild(gr);

			// generate text if the rectangle is sufficiently wide
			var textel = null;
			if (typeof nodeInfo[iterStack[i].id] !== 'undefined' && iterStack[i].flameGraphWidth > borderWidth*2 + boxh)
			{
				// create text element
				textel = svgDoc.createElementNS(svgns, 'text');
				textel.setAttributeNS(null, 'x', iterStack[i].flameGraphLeft + boxh *0.2);
				textel.setAttributeNS(null, 'y', baseline + boxh*0.65);
				textel.setAttributeNS(null, 'font-family', 'Courier New');
				textel.setAttributeNS(null, 'font-size', Math.round(boxh*0.55));
				textel.setAttributeNS(null, 'color', '#000000');
				textel.setAttributeNS(null, 'style', 'cursor:pointer;');
				textel.textContent = nodeInfo[iterStack[i].id].name;
				gr.appendChild(textel);

				// now verify text length
				var sz = textel.getComputedTextLength();
				var textRefSize = iterStack[i].flameGraphWidth - 2*boxh*0.2;

				// and in case when the text is longer, shorten it
				var initialRat = textRefSize/sz;
				if (initialRat < 1.0)
				{
					// at first, shorten it using width ratio
					textel.textContent = textel.textContent.substr(0, textel.textContent.length*initialRat - 3) + '...';
					sz = textel.getComputedTextLength();
					var safeness = 5;
					// shorten more, if needed
					while (sz > textRefSize)
					{
						textel.textContent = textel.textContent.substr(0, textel.textContent.length - 2 - 3) + '...';
						sz = textel.getComputedTextLength();
						// when safeness counter drops to zero, that means we didn't succeed
						if (safeness-- <= 0)
						{
							textel.textContent = '';
							break;
						}
					}

					// just three dots in name is meaningless, erase it
					if (textel.textContent == '...')
						textel.textContent = '';
				}
			}

			// mouse enter event
			$(gr).mouseenter(function() {
				var nodename = '??';
				if (typeof nodeInfo[$(this).attr('nodeid')] !== 'undefined')
					nodename = nodeInfo[$(this).attr('nodeid')].name;

				// show tooltip
				floatingTooltip.show();
				floatingTooltip.css('margin-top', '-6.5em');
				floatingTooltip.html('<span class="fname">'+escapeHtml(nodename)+'</span><br />'+
									 'Sample count: '+$(this).attr('samples')+'<br />'+
									 'Inclusive time: '+$(this).attr('time-total-inclusive-pct')+'% ('+$(this).attr('time-total-inclusive')+'s)<br />'+
									 'Exclusive time: '+$(this).attr('time-total-exclusive-pct')+'% ('+$(this).attr('time-total-exclusive')+'s)');
			});
			// mouse leave event
			$(gr).mouseleave(function() {
				// hide and reset tooltip
				floatingTooltip.hide();
				floatingTooltip.css('margin-top', '');
			});
			// mouse click event
			$(gr).click(function() {
				// expand flame graph from point clicked
				pivo_createFlameGraph($(this).attr('call-tree-path'));
			});
		}

		// swap stacks
		iterStack = tmpStack;
		tmpStack = [];

		// decrease baseline and generate next level
		baseline -= (boxh + spaceh);
	}
}

function pivo_getHexColor(r, g, b)
{
	var color = '#';

	color += ('00' + Number(r).toString(16)).slice(-2)
		+('00' + Number(g).toString(16)).slice(-2)
		+('00' + Number(b).toString(16)).slice(-2);
	
	return color;
}

function rangeAlignColor(v)
{
	if (v > 255)
		return 255;
	if (v < 0)
		return 0;
	return v;
}

function pivo_toolbarHide(filterclass, toolbarclass)
{
	$('.toolbar-'+toolbarclass+'.'+filterclass).hide();
	$('.toolbar-'+toolbarclass+'.opener.'+filterclass).show();
}

function pivo_toolbarShow(filterclass, toolbarclass)
{
	$('.toolbar-'+toolbarclass+'.opener.'+filterclass).hide();
	$('.toolbar-'+toolbarclass+'.'+filterclass).not('.opener').show();
}

function pivo_callGraph_expandSubtreeLevels(nodeId)
{
	if (nodeInfo[nodeId].hitCount > 5)
		return;
	if (!nodeInfo[nodeId].present)
		return;
	nodeInfo[nodeId].hitCount++;

	nodeInfo[nodeId].traversalState = 1;

	for (var i in incidencyGraph[nodeId])
	{
		var childNodeId = incidencyGraph[nodeId][i];
		if (typeof nodeInfo[childNodeId].level === 'undefined' || nodeInfo[childNodeId].level === null || (nodeInfo[childNodeId].level <= nodeInfo[nodeId].level && childNodeId != nodeId))
		{
			if (nodeInfo[childNodeId].traversalState != 1)
			{
				nodeInfo[childNodeId].level = nodeInfo[nodeId].level + 1;
				pivo_callGraph_expandSubtreeLevels(childNodeId);
			}
		}
	}

	nodeInfo[nodeId].traversalState = 2;
}

function pivo_callGraph_calculateSubtreeLevels(startNodes)
{
	for (var i in startNodes)
		pivo_callGraph_expandSubtreeLevels(startNodes[i]);
}

function pivo_removeManualEntryPoint()
{
	$('.callgraph-preference-manualentry').val(-1);
	pivo_createCallGraph();
}

function pivo_createCallGraph()
{
	$('#call-graph-loading').show();
	$('#call-graph-loading .bar .complete').css({ width: 0 });
	
	var entryPoint;
	var entrypointval = parseInt($('.callgraph-preference-manualentry').val());

	if (entrypointval >= 0)
	{
		entryPoint = entrypointval;
		
		var nam = nodeInfo[entryPoint].name;
		if (nam.length > 24)
			nam = nam.substr(0, 24)+'...';
		
		$('.callgraph-preference-manualentry-name').text(nam);
		$('.callgraph-preference-manualentry-name').attr('title', nodeInfo[entryPoint].name);
		$('#callgraph-preference-remove-manualentry').show();
	}
	else
	{
		entryPoint = undefined;
		$('.callgraph-preference-manualentry-name').text('-');
		$('.callgraph-preference-manualentry-name').attr('title', 'Double click on node to set manual entry point');
		$('#callgraph-preference-remove-manualentry').hide();
	}

	// reset graph state
	for (var i in nodeInfo)
	{
		// by default it's not present in graph
		nodeInfo[i].present = false;
		// also reset level info
		nodeInfo[i].level = null;
		// nullify hitcount
		nodeInfo[i].hitCount = 0;
		// reset traversal state
		nodeInfo[i].traversalState = 0;
	}

	var container = document.getElementById('callgraph_canvas');
	
	var filterConditions = [];
	$.each($('.callgraph-filter-entry'), function(i,e) {
		var type = $(e).find('.callgraph-filter-type').val();
		var op = $(e).find('.callgraph-filter-operator').val();
		var value = $(e).find('.callgraph-filter-value').val();
		
		if (type !== 'none')
			filterConditions.push({ 'type': type, 'op': op, 'value': value });
	});

	var preferenceNameEllipsis = parseInt($('.callgraph-preference-nameellipsis').val());
	if (typeof preferenceNameEllipsis === 'undefined' || preferenceNameEllipsis == null)
		preferenceNameEllipsis = 0;

	var presentNodes = [];
	for (var i in nodeInfo)
	{
		var node = nodeInfo[i];
		var passed = true;

		for (var j in filterConditions)
		{
			if (!(filterOperatorPredicates[filterConditions[j].op](filterSourceFieldPredicates[filterConditions[j].type](node), filterConditions[j].value)))
			{
				passed = false;
				break;
			}
		}

		if (!passed)
			continue;

		node.label = node.name;
		if (preferenceNameEllipsis > 0 && node.label.length > preferenceNameEllipsis)
			node.label = node.label.substr(0, preferenceNameEllipsis) + '..';

		nodeInfo[i].present = true;
		presentNodes.push(node.id);
	}
	
	// determine input and output degree based on incidencyGraph and reverseIncidencyGraph arrays, and "present" node field
	for (var i in nodeInfo)
	{
		nodeInfo[i].inputDegree = 0;
		nodeInfo[i].outputDegree = 0;
		
		for (var j in reverseIncidencyGraph[i])
		{
			if (nodeInfo[reverseIncidencyGraph[i][j]].present)
				nodeInfo[i].inputDegree++;
		}
		for (var j in incidencyGraph[i])
		{
			if (nodeInfo[incidencyGraph[i][j]].present)
				nodeInfo[i].outputDegree++;
		}
	}

	// prepare start nodes
	var startNodes = [];

	var useTextEntryPoint = $('.callgraph-preference-textentrypoint').is(':checked');

	if (typeof entryPoint === 'undefined')
	{
		// keep resolving call graph hierarchy until every node is traversed
		do
		{
			startNodes = [];
			// find entry point candidates and push them into array
			// 1. all functions with zero input degree (are not called throughout execution, just call others)
			for (var i in reverseIncidencyGraph)
			{
				if (nodeInfo[i].present && nodeInfo[i].traversalState == 0 && nodeInfo[i].inputDegree == 0 && (!useTextEntryPoint || (useTextEntryPoint && nodeInfo[i].functionType == 't')))
					startNodes.push(i);
			}
			// 2. functions called "main", and, for safeness reasons, greater (or equal) output degree than input degree
			for (var i in nodeInfo)
			{
				var nm = nodeInfo[i].name;
				if (nodeInfo[i].present && nodeInfo[i].traversalState == 0 && (!useTextEntryPoint || (useTextEntryPoint && nodeInfo[i].functionType == 't')) && startNodes.indexOf(i) == -1 && (nm == "main" || nm == "WinMain" || nm == "__main" || nm == "_main") && nodeInfo[i].inputDegree <= nodeInfo[i].outputDegree)
					startNodes.push(i);
			}
			// 3. node with maximum output degree and minimum input degree
			if (startNodes.length == 0)
			{
				var maxRatio = 0;
				var maxRatioId = -1;
				for (var i in nodeInfo)
				{
					if (nodeInfo[i].inputDegree == 0 || !nodeInfo[i].present || nodeInfo[i].traversalState !== 0 || (useTextEntryPoint && nodeInfo[i].functionType !== 't'))
						continue;

					var rt = nodeInfo[i].outputDegree / nodeInfo[i].inputDegree;
					if (rt > maxRatio)
					{
						maxRatio = rt;
						maxRatioId = i;
					}
				}
				
				if (maxRatioId > -1)
					startNodes.push(maxRatioId);
			}

			pivo_callGraph_calculateSubtreeLevels(startNodes);
		
		} while (startNodes.length !== 0);
	}
	else
	{
		startNodes.push(entryPoint);
		
		// for each node in start nodes, set level = 0, because they're our root nodes
		for (var i in startNodes)
			nodeInfo[startNodes[i]].level = 0;

		pivo_callGraph_calculateSubtreeLevels(startNodes);
	}

	// push active nodes into graph nodes array
	var convNodes = [];
	for (var i in nodeInfo)
	{
		var node = nodeInfo[i];
		if (typeof node.level === 'undefined' || node.level === null)
		{
			if (typeof entryPoint !== 'undefined' || (useTextEntryPoint && node.functionType !== 't'))
			{
				node.present = false;
				continue;
			}
			else
				node.level = 0;
		}

		if (node.present)
			convNodes.push(node);
	}

	// push edges between existing and active graph nodes into graph edges array
	var convEdges = [];
	for (var i in callGraphData)
	{
		if (presentNodes.indexOf(callGraphData[i].from) !== -1 &&
			presentNodes.indexOf(callGraphData[i].from) !== -1)
		{
			convEdges.push(callGraphData[i]);
		}
	}
	
	// determine color
	var colorBy = $('.callgraph-preference-color-source').val();
	var colMinVal, colMaxVal;
	
	colMinVal = null;
	colMaxVal = null;
	for (var i in nodeInfo)
	{
		// we determine coloring range only from visible nodes
		if (!nodeInfo[i].present)
			continue;

		v = (filterSourceFieldPredicates[colorBy](nodeInfo[i]));
		if (colMinVal === null || v < colMinVal)
			colMinVal = v;
		if (colMaxVal === null || v > colMaxVal)
			colMaxVal = v;
	}

	var colValDiff = colMaxVal - colMinVal;
	for (var i in nodeInfo)
	{
		var pct = colValDiff > 0 ? (filterSourceFieldPredicates[colorBy](nodeInfo[i]) - colMinVal) / colValDiff : 1;
		var r, g;
		if (pct > 0.5)
		{
			r = 255;
			g = rangeAlignColor(Math.floor(255 * (0.5 - (pct - 0.5)) * 2));
		}
		else
		{
			r = rangeAlignColor(Math.ceil(255 * pct * 2));
			g = 255;
		}
		var b = 0;
		nodeInfo[i].color = pivo_getHexColor(r,g,b);
	}
	
	// build graph data object
	var data = { nodes: convNodes, edges: convEdges };

	// graph layout options
	var layoutOptions = {
		randomSeed: 2
	};
	
	var graphNodeSpacing = preferenceNameEllipsis > 0 ? (150*preferenceNameEllipsis/16) : 200;

	// determine node hierarchy status
	var hierarchyEnabled = $('.callgraph-preference-hierarchy').is(':checked');
	if (hierarchyEnabled)
	{
		layoutOptions['hierarchical'] = {
			enabled: true,
			direction: 'UD',
			sortMethod: 'directed',
			nodeSpacing: graphNodeSpacing,
		};
	}
	
	var options = {
		layout: layoutOptions,
		nodes: {
			shape: 'box',
			size: 8,
			font: {
				size: 14,
				color: '#000000'
			},
			borderWidth: 2
		},
		physics: {
			enabled: true,
			stabilization: {
				enabled: true,
				iterations: 5000,
				updateInterval: 121
			}
		},
		edges: {
			width: 2,
			smooth: {
				type: 'dynamic'
			}
		},
		interaction:  {
			hover: true,
			tooltipDelay: 0
		}
	};
	
	var network = new vis.Network(container, data, options);
	
	network.on("doubleClick", function (params) {
		if (typeof params.nodes !== 'undefined' && params.nodes.length === 1)
		{
			$('.callgraph-preference-manualentry').val(params.nodes[0]);
			pivo_createCallGraph();
		}
	});

	$('#call-graph-loading .bar .complete').css({ width: 0 });

	network.on("stabilizationProgress", function(params) {
		var wf = 100.0*params.iterations/params.total;
		$('#call-graph-loading .bar .complete').css({ width: wf+'%' });
		$('#call-graph-loading .text').text('Loading... '+params.iterations+'/'+params.total+' ('+Math.round(wf)+'%)');
	});

	network.once("stabilizationIterationsDone", function() {
		$('#call-graph-loading').hide();
		options.physics.enabled = false;
		network.setOptions(options);
	});
}
