
function pivo_selectTab(identifier)
{
	$('.report-tab-content').addClass('hidden');
	$('.report-tab').removeClass('selected');
	
	$('#report-tab--'+identifier).removeClass('hidden');
	$('#report-tab-menu--'+identifier).addClass('selected');
}

var nodeInfo = {
};

var callGraphData = [
];

function pivo_addCallGraphNode(id, name, timeTotal, timeTotalPct)
{
	if (typeof nodeInfo[id] === 'undefined')
		nodeInfo[id] = { 'id': id, 'label': name, 'profTimeTotal': timeTotal, 'profTimePct': timeTotalPct };
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

function pivo_createCallGraph()
{
	var rootNodeCandidates = [];

	for (var i in nodeInfo)
	{
		var pct = nodeInfo[i].profTimePct / 100.0;
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
	var convNodes = Object.keys(nodeInfo).map(function (key) {return nodeInfo[key]});
	var data = { nodes: convNodes, edges: callGraphData };
	
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
				color: '#000000',
				background: '#FFFFFF'
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
