import React, { Component } from "react";
import CustomListDropDown from "./Dropdown";
import { DataContext } from "./Context";

class VM extends Component {
    
    static contextType = DataContext;

    state = {
        iframe_key: 0,
        iframe_url: 'http://127.0.0.1:5001/remote/ssh/adina/22'
    }

    iframeRefresh() {
        this.setState({iframe_key: this.state.iframe_key + 1});
        console.log(this.state.iframe_key)
    }

    start = () => {
        this.context.startVM();
    } 

    render () {
        const isStarted = this.context.isVMStarted;

        if (isStarted === false) {
        return(
            <div className="container-fluid">
                <div>
                    <div className = "card mt-2 row shadowalign-items-start" style = {{ width: `65%`, backgroundColor: `F6F5F5`, paddingBottom: `1rem` }}>
                        <div className="card-body col text-center">
                            <img src = {require("../images/vm.png")} class = "img-fluid" alt="..." />
                        </div>
                        <CustomListDropDown />
                        <button type="button" onClick = {() => this.start()} class="btn shadow" style = {{ marginTop: `2rem`, width: `40%`, marginLeft: `6rem`, backgroundColor:`#276678`, color:`white` }}>Start</button>
                    </div>
                </div>
            </div>
        );
        }
        else {
            return(
            <div className="container" style= {{ minWidth: `50rem`, marginLeft:`1.5rem`, marginTop:`1rem` }} >
                <div>
                    <button type="button" class="btn-sm shadow row" onClick = {() => { this.iframeRefresh(); }} style= {{ backgroundColor:`#276678`, color:`white` }}>Reload terminal</button>
                    <iframe class="row" src = { this.state.iframe_url } key = { this.state.iframe_key } width = "96%" height = "550"  />
                </div>
            </div>)

        }
    }
}

export default VM;