import React from 'react';

import Home from './components/pages/Home';
import About from './components/pages/About';
import Syscalls from './components/pages/Syscalls';
import { DataProvider } from './components/includes/Context';
import Navbar from './components/includes/Navbar';

import { BrowserRouter as Router, Route, Routes } from 'react-router-dom'

function App() {
  return (
    <Router>
      <DataProvider>
        <div>
          <Navbar />
          <Routes>
            <Route path = "/" element={<Home />} />
            <Route path = "/about" element={<About />} />
            <Route path = "/syscalls" element={<Syscalls />} />
          </Routes>
        </div>
      </DataProvider>
  </Router>
      
  );
}

export default App;
