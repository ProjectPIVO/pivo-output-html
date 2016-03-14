
var filterSourceFieldPredicates = {
	'none': function(node) { return 0; },
	'inclusive': function(node) { return node.profTimeTotalInclusive; },
	'inclusive-pct': function(node) { return node.profTimeTotalInclusivePct; },
	'exclusive': function(node) { return node.profTimeTotalExclusive; },
	'exclusive-pct': function(node) { return node.profTimeTotalExclusivePct; },
	'callcount': function(node) { return node.profTotalCallCount; },
};

var filterOperatorPredicates = {
	'greaterthan': function(a,b) { return a > b; },
	'lessthan': function(a,b) { return a < b; },
	'equals': function(a,b) { return a == b; }
};

function pivo_selectTab(identifier)
{
	$('.report-tab-content').addClass('hidden');
	$('.report-tab').removeClass('selected');
	
	$('#report-tab--'+identifier).removeClass('hidden');
	$('#report-tab-menu--'+identifier).addClass('selected');
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

function pivo_addCallGraphNode(id, name, timeTotalInclusive, timeTotalInclusivePct, timeTotal, timeTotalPct, totalCallCount)
{
	if (typeof nodeInfo[id] === 'undefined')
	{
		nodeInfo[id] = {
			'id': id,
			'name': name,
			'profTimeTotalInclusive': timeTotalInclusive,
			'profTimeTotalInclusivePct': timeTotalInclusivePct,
			'profTimeTotalExclusive': timeTotal,
			'profTimeTotalExclusivePct': timeTotalPct,
			'profTotalCallCount': totalCallCount
		};
	}
}

function pivo_addCallGraphEdge(source, dest, callCount)
{
	callGraphData.push({ 'from': source, 'to': dest, 'arrows': 'to' });
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

function pivo_createCallGraph()
{
	var colorBy = $('.callgraph-preference-color-source').val();
	var colMinVal = 0.0, colMaxVal = 100.0;
	
	colMinVal = null;
	colMaxVal = null;
	for (var i in nodeInfo)
	{
		v = (filterSourceFieldPredicates[colorBy](nodeInfo[i]));
		if (colMinVal === null || v < colMinVal)
			colMinVal = v;
		else if (colMaxVal === null || v > colMaxVal)
			colMaxVal = v;
	}

	var colValDiff = colMaxVal - colMinVal;
	for (var i in nodeInfo)
	{
		var pct = (filterSourceFieldPredicates[colorBy](nodeInfo[i]) - colMinVal) / colValDiff;
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
	
	var data = { nodes: convNodes, edges: convEdges };
	
	var options = {
		layout: {
			randomSeed: 2,
//			hierarchical: {
//				direction: 'UD',
//				sortMethod: 'directed'
//				nodeSpacing: 200
//			}
		},
		nodes: {
			shape: 'box',
			size: 8,
			font: {
				size: 10,
				color: '#000000'
			},
			borderWidth: 2
		},
		physics: {
			enabled: false
		},
		edges: {
			width: 2,
			smooth: {type:'cubicBezier'}
		}
	};

	var network = new vis.Network(container, data, options);
}
