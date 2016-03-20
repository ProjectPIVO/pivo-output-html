
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

function escapeHtml(txt)
{
	return $('<div>').text(txt).html();
}

function pivo_addCallGraphNode(id, name, timeTotalInclusive, timeTotalInclusivePct, timeTotal, timeTotalPct, totalCallCount)
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
			'title': title
		};

		incidencyGraph[id] = [];
		reverseIncidencyGraph[id] = [];
	}
}

function pivo_addCallGraphEdge(source, dest, callCount)
{
	callGraphData.push({ 'from': source, 'to': dest, 'arrows': 'to' });

	incidencyGraph[source].push(dest);
	reverseIncidencyGraph[dest].push(source);
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

function pivo_createCallGraph(entryPoint)
{
	// reset graph state
	for (var i in nodeInfo)
	{
		// by default it's not present in graph
		nodeInfo[i].present = false;
		// also reset level info
		nodeInfo[i].level = null;
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

	// TODO: recursion
	var dfsStack = [];

	if (typeof entryPoint === 'undefined')
	{
		// find entry point candidates and push them onto stack
		// 1. all functions with zero input degree (are not called throughout execution, just call others)
		for (var i in reverseIncidencyGraph)
		{
			if (reverseIncidencyGraph[i].length == 0)
				dfsStack.push(i);
		}
		// 2. functions called "main", and, for safeness reasons, greater output degree than input degree
		for (var i in nodeInfo)
		{
			var nm = nodeInfo[i].name;
			if (dfsStack.indexOf(i) == -1 && (nm == "main" || nm == "WinMain" || nm == "__main" || nm == "_main") && reverseIncidencyGraph[i].length < incidencyGraph[i].length)
				dfsStack.push(i);
		}
		// 3. fallback - when no node in DFS stack, attempt to make graph traversal to determine group of nodes, which should serve as start nodes to traverse whole graph
		if (dfsStack.length == 0)
		{
			// TODO: make this real
		}
	}
	else
	{
		dfsStack.push(entryPoint);
	}
	
	// for each node in DFS stack, set level = 0, because they're our root nodes
	for (var i in dfsStack)
		nodeInfo[dfsStack[i]].level = 0;

	// DFS traversal for each node in stack
	while (dfsStack.length > 0)
	{
		var nodeId = dfsStack.pop();

		for (var i in incidencyGraph[nodeId])
		{
			var childNodeId = incidencyGraph[nodeId][i];
			if (typeof nodeInfo[childNodeId].level === 'undefined' || nodeInfo[childNodeId].level === null || nodeInfo[childNodeId].level < nodeInfo[nodeId].level)
			{
				nodeInfo[childNodeId].level = nodeInfo[nodeId].level + 1;
				dfsStack.push(childNodeId);
			}
		}
	}

	var preferenceNameEllipsis = parseInt($('.callgraph-preference-nameellipsis').val());
	if (typeof preferenceNameEllipsis === 'undefined' || preferenceNameEllipsis == null)
		preferenceNameEllipsis = 0;

	var presentNodes = [];
	var convNodes = [];
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

		if (typeof node.level === 'undefined' || node.level === null)
		{
			if (typeof entryPoint === 'undefined')
				node.level = 0;
			else
				continue;
		}
		
		nodeInfo[i].present = true;
		convNodes.push(node);
		presentNodes.push(node.id);
	}
	
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
			direction: 'UD',
			sortMethod: 'directed',
			nodeSpacing: graphNodeSpacing
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
			enabled: false
		},
		edges: {
			width: 2
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
			pivo_createCallGraph(params.nodes[0]);
		}
	});
}
