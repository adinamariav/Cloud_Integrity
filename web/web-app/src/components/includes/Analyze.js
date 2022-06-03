import React, { Component } from "react";
import TimePicker from 'react-time-picker';
import { DataContext } from "./Context";

class Analyze extends Component  {
    static contextType = DataContext;

    render () {
        return(
            <div className="container">
                <div>
                    <div className="card mt-4 row shadow border-info align-items-start" style= {{ width: `50rem`, backgroundColor: `lavender` }}>
                        <h1>Analyze</h1>
                        <p>
                            Blalalalalalallalllalalalalalalalalalalalalalalalalala
                        </p>
                        <p>
                            Blalalalalalallalllalalalalalalalalalalalalalalalalala
                        </p>

                        <TimePicker disableClock= { true } onChange = { value => this.context.analyzeTimeChanged(value)} />

                        <div className="col text-center">
                            <button type="button" class="btn btn-danger shadow" onClick={ () => {alert(`Running for: ${this.context.analyzeTime} seconds`)}} style= {{ width: `5rem`, marginTop: `1rem`, marginBottom: `1rem` }}>Run</button>
                        </div>
                        
                    </div>
                </div>
            </div>
        );
    }
}

export default Analyze;