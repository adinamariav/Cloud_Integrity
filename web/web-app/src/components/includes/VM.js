import React, { Component } from "react";
import CustomListDropDown from "./Dropdown";
import { DataContext } from "./Context";

class VM extends Component {
    
    static contextType = DataContext;

    start = () => {
        this.context.startVM();
    } 

    render () {
        const isStarted = this.context.isVMStarted;

        if (isStarted === false) {
        return(
            <div className="container-fluid">
                <div>
                    <div className="card mt-2 row shadowalign-items-start" style= {{ width: `65%`, backgroundColor: `F6F5F5`, paddingBottom: `1rem` }}>
                        <div className="card-body col text-center">
                            <img src= {require("../images/vm.png")} class="img-fluid" alt="..." />
                        </div>
                        <CustomListDropDown />
                        <button type="button" onClick={() => this.start()} class="btn shadow" style= {{ marginTop: `2rem`, width: `40%`, marginLeft: `6rem`, backgroundColor:`#276678`, color:`white`}}>Start</button>
                    </div>
                </div>
            </div>
        );
        }
        else {
            return(
            <div className="container" style= {{ minWidth: `50rem` }} >
                <div>
                    <iframe src = 'http://127.0.0.1:5001/remote/ssh/adina/22' width = "96%" height = "550"  />
                </div>
            </div>)

        }
    }
}

export default VM;