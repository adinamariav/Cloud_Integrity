import React, { Component } from "react";
import { DataContext } from './Context';

class SyscallInput extends Component {

    static contextType = DataContext;

    handleChange = (event) => {
        this.context.setSyscalls(event.target.value);
    }

    render () {
        const { isVMStarted } = this.context;
        const { isSandboxStarted } = this.context;

        if (isVMStarted && !isSandboxStarted) {
            return (
                <div class="input-group row mt-5" style={{width:`104%`}}>
                    <span class="input-group-text">Enter desired syscalls here, separated by space:</span>
                    <textarea class="form-control" onChange = { (value) => this.handleChange(value) } ></textarea>
                </div>
            )
        }
    }
}

export default SyscallInput