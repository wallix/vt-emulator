var TTYHTMLRendering = (function(){

var i2strcolor = function(int_color)
{
    return ('00000' + int_color.toString(16)).slice(-6)
}

var elem2style = function(e)
{
    var style = '';
    if (e.f) style += 'color:#'+i2strcolor(e.f)+';';
    if (e.b) style += 'background-color:#'+i2strcolor(e.b)+';';
    if (e.r) {
        if (e.r & 1) style += 'font-weight:bold;';
        if (e.r & 2) style += 'font-style:italic;';
        if (e.r & 4) style += 'text-decoration:underline;';
    }
    return style
}

var escaped = function(s) {
    return s.replace(/&/g, '&amp;').replace(/</g, '&lt;')
}

return function(screen)
{
    var estyle = {r: screen.style.r, f: screen.style.f, b: screen.style.b};

    var empty_line = '                                                               '
    while (empty_line.length < screen.columns) {
        empty_line += empty_line;
    }

    var terminal = '';
    var data = screen.data;
    //console.log(screen.x,screen.y)
    for (var ilines in data) {
        var htmlline = ''
        var lines = data[ilines]
        for (var iline in lines) {
            var line_len = 0
            var line = lines[iline]
            var is_cursor_y = (ilines == screen.y)
            for (var ie in line) {
                var e = line[ie]
                //console.log(e)
                if (e.r !== undefined) estyle.r = e.r;
                if (e.f !== undefined) estyle.f = e.f;
                if (e.b !== undefined) estyle.b = e.b;

                var span_style = '<span style="' + elem2style(estyle) + '">'
                var s = e.s || ''
                if (is_cursor_y && line_len <= screen.x && screen.x <= s.length + line_len) {
                    var curstyle = {r: estyle.r, f: estyle.b, b: estyle.f}
                    var span_style = '<span style="' + elem2style(estyle) + '">'
                    var span_cur_style = '<span style="' + elem2style(curstyle) + '">'
                    var pcur = screen.x - line_len;
                    htmlline += span_style + escaped(s.substr(0, pcur)) + '</span>' +
                        span_cur_style + escaped(s.substr(pcur, 1)) + '</span>' +
                        span_style + escaped(s.substr(pcur + 1)) + '</span>'
                }
                else {
                    htmlline += span_style + escaped(s) + '</span>'
                }
                line_len += s.length
            }
        }
        terminal += '<p>' + htmlline + '\n</p>';
    }

    return '<p id="tty-player-title">' + (screen.title||'') + '</p>'+
        '<div id="tty-player-terminal" style="color:' +
            i2strcolor(screen.style.f) + ';background-color:' +
            i2strcolor(screen.style.b) + '">' +
            terminal +
            // force terminal width
            '<p style="height:1px">' + empty_line.substr(0, screen.columns) + '</p>' +
        '</div>'
    ;
}

})()
