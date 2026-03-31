import dash

from current_callbacks import register_callbacks
from current_views import create_layout


app = dash.Dash(__name__, suppress_callback_exceptions=True, serve_locally=True)
app.index_string = '''
<!DOCTYPE html>
<html>
    <head>
        {%metas%}
        <title>Sheath Circulating Current Monitoring</title>
        {%favicon%}
        {%css%}
        <style>
            @keyframes fadeOutMessage {
                0% { opacity: 0; transform: translateY(-3px); }
                10% { opacity: 1; transform: translateY(0); }
                80% { opacity: 1; transform: translateY(0); }
                100% { opacity: 0; transform: translateY(-3px); }
            }
            @keyframes flashRedBg {
                0% { background-color: transparent; }
                50% { background-color: rgba(255, 0, 0, 0.7); }
                100% { background-color: transparent; }
            }
            .alarm-flash {
                animation: flashRedBg 1s infinite;
                border-radius: 4px;
            }
        </style>
    </head>
    <body>
        {%app_entry%}
        <footer>
            {%config%}
            {%scripts%}
            {%renderer%}
        </footer>
    </body>
</html>
'''
app.layout = create_layout()
register_callbacks(app)


if __name__ == "__main__":
    app.run(debug=False, host="0.0.0.0", port=8050)
